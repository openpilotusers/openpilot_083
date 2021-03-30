import math
import numpy as np

from cereal import log
import cereal.messaging as messaging


from cereal import log
import cereal.messaging as messaging
from selfdrive.config import Conversions as CV
from selfdrive.controls.lib.speed_smoother import speed_smoother
from selfdrive.controls.lib.long_mpc import LongitudinalMpc
from selfdrive.controls.lib.lane_planner import TRAJECTORY_SIZE
from selfdrive.car.hyundai.values import Buttons
from common.numpy_fast import clip, interp

from selfdrive.config import RADAR_TO_CAMERA


import common.log as trace1
import common.CTime1000 as tm
import common.MoveAvg as moveavg1




MAX_SPEED = 255.0
MIN_CURVE_SPEED = 30.

LON_MPC_STEP = 0.2  # first step is 0.2s
MAX_SPEED_ERROR = 2.0
AWARENESS_DECEL = -0.2     # car smoothly decel at .2m/s^2 when user is distracted

# lookup tables VS speed to determine min and max accels in cruise
# make sure these accelerations are smaller than mpc limits
_A_CRUISE_MIN_V = [-1.0, -.8, -.67, -.5, -.30]
_A_CRUISE_MIN_BP = [0., 5.,  10., 20.,  40.]

# need fast accel at very low speed for stop and go
# make sure these accelerations are smaller than mpc limits
_A_CRUISE_MAX_V = [1.2, 1.2, 0.65, .4]
_A_CRUISE_MAX_V_FOLLOWING = [1.6, 1.6, 0.65, .4]
_A_CRUISE_MAX_BP = [0.,  6.4, 22.5, 40.]

# Lookup table for turns
_A_TOTAL_MAX_V = [1.7, 3.2]
_A_TOTAL_MAX_BP = [20., 40.]

# 75th percentile
SPEED_PERCENTILE_IDX = 7

def limit_accel_in_turns(v_ego, angle_steers, a_target, steerRatio, wheelbase):
    """
    This function returns a limited long acceleration allowed, depending on the existing lateral acceleration
    this should avoid accelerating when losing the target in turns
    """

    a_total_max = interp(v_ego, _A_TOTAL_MAX_BP, _A_TOTAL_MAX_V)
    a_y = v_ego**2 * angle_steers * CV.DEG_TO_RAD / (steerRatio * wheelbase)
    a_x_allowed = math.sqrt(max(a_total_max**2 - a_y**2, 0.))

    return [a_target[0], min(a_target[1], a_x_allowed)]


class SpdController():
    def __init__(self, CP=None):
        self.long_control_state = 0  # initialized to off

        self.seq_step_debug = 0
        self.long_curv_timer = 0

        self.path_x = np.arange(192)

        self.traceSC = trace1.Loger("SPD_CTRL")

        self.wheelbase = 2.845
        self.steerRatio = 12.5  # 12.5

        self.v_model = 0
        self.a_model = 0
        self.v_cruise = 0
        self.a_cruise = 0

        self.l_poly = []
        self.r_poly = []

        self.movAvg = moveavg1.MoveAvg()
        self.Timer1 = tm.CTime1000("SPD")
        self.time_no_lean = 0

        self.wait_timer2 = 0

        self.cruise_set_speed_kph = 0
        self.curise_set_first = 0
        self.curise_sw_check = 0
        self.prev_clu_CruiseSwState = 0    

        self.prev_VSetDis  = 0

        self.cruise_set_mode = 0
        self.sc_clu_speed = 0
        self.btn_type = Buttons.NONE
        self.active_time = 0

        self.old_model_speed = 0
        self.old_model_init = 0

        self.curve_speed = 0
        self.curvature_gain = 1
        

    def reset(self):
        self.v_model = 0
        self.a_model = 0
        self.v_cruise = 0
        self.a_cruise = 0



    def cal_curve_speed(self, sm, v_ego, frame):
        #if frame % 10 == 0:
        md = sm['modelV2']
        if len(md.position.x) == TRAJECTORY_SIZE and len(md.position.y) == TRAJECTORY_SIZE:
            x = md.position.x
            y = md.position.y
            dy = np.gradient(y, x)
            d2y = np.gradient(dy, x)
            curv = d2y / (1 + dy ** 2) ** 1.5
            curv = curv[5:TRAJECTORY_SIZE-10]
            a_y_max = 2.975 - v_ego * 0.0375  # ~1.85 @ 75mph, ~2.6 @ 25mph
            v_curvature = np.sqrt(a_y_max / np.clip(np.abs(curv), 1e-4, None))
            model_speed = np.mean(v_curvature) * 0.9 * self.curvature_gain
            self.curve_speed = float(max(model_speed * CV.MS_TO_KPH, MIN_CURVE_SPEED))
            if np.isnan(self.curve_speed):
                self.curve_speed = MAX_SPEED

            if self.curve_speed > MAX_SPEED:
               self.curve_speed = MAX_SPEED                
        else:
            self.curve_speed = MAX_SPEED
    
        return  self.curve_speed

    def calc_laneProb(self, prob, v_ego):
        if len(prob) > 1:
            path = list(prob)

            # TODO: compute max speed without using a list of points and without numpy
            y_p = 3 * path[0] * self.path_x**2 + \
                2 * path[1] * self.path_x + path[2]
            y_pp = 6 * path[0] * self.path_x + 2 * path[1]
            curv = y_pp / (1. + y_p**2)**1.5

            #print( 'curv = {}'.format( curv) )

            a_y_max = 2.975 - v_ego * 0.0375  # ~1.85 @ 75mph, ~2.6 @ 25mph
            v_curvature = np.sqrt(a_y_max / np.clip(np.abs(curv), 1e-4, None))
            model_speed = np.min(v_curvature)
            # Don't slow down below 20mph

            model_speed *= CV.MS_TO_KPH
            model_speed = max(MIN_CURVE_SPEED, model_speed)
           # print( 'v_curvature = {}'.format( v_curvature) )
            #print( 'model_speed = {}  '.format( model_speed) )

            
            if model_speed > MAX_SPEED:
               model_speed = MAX_SPEED
        else:
            model_speed = MAX_SPEED
          

        return  model_speed


    def cal_model_speed(self, sm, v_ego):
        md = sm['modelV2']
        #print('{}'.format( md ) )
        if len(md.path.poly):
            self.prob = list(md.path.poly)

            model_speed = self.calc_laneProb( self.prob, v_ego )
    
            delta_model = model_speed - self.old_model_speed
            if self.old_model_init < 10:
                self.old_model_init += 1
                self.old_model_speed = model_speed
            elif self.old_model_speed == model_speed:
                pass
            elif delta_model < -1:
                self.old_model_speed -= 0.5  #model_speed
            elif delta_model > 0:
                self.old_model_speed += 0.1

            else:
                self.old_model_speed = model_speed

        return self.old_model_speed


    def update_cruiseSW(self, CS ):
        set_speed_kph = self.cruise_set_speed_kph
        delta_vsetdis = 0
        if CS.acc_active:
            delta_vsetdis = abs(CS.VSetDis - self.prev_VSetDis)
            if self.prev_clu_CruiseSwState != CS.cruise_buttons:
                if CS.cruise_buttons:
                    self.prev_VSetDis = int(CS.VSetDis)
                elif CS.driverOverride:
                    set_speed_kph = int(CS.VSetDis)          
                elif self.prev_clu_CruiseSwState == Buttons.RES_ACCEL:   # up 
                    if self.curise_set_first:
                        self.curise_set_first = 0
                        set_speed_kph =  int(CS.VSetDis)
                    elif delta_vsetdis > 5:
                        set_speed_kph = CS.VSetDis
                    elif not self.curise_sw_check:
                        set_speed_kph += 1
                elif self.prev_clu_CruiseSwState == Buttons.SET_DECEL:  # dn
                    if self.curise_set_first:
                        self.curise_set_first = 0
                        set_speed_kph = int(CS.clu_Vanz)
                    elif delta_vsetdis > 5:
                        set_speed_kph = int(CS.VSetDis)
                    elif not self.curise_sw_check:
                        set_speed_kph -= 1

                self.prev_clu_CruiseSwState = CS.cruise_buttons
            elif CS.cruise_buttons and delta_vsetdis > 0:
                self.curise_sw_check = True
                set_speed_kph = int(CS.VSetDis)
        else:
            self.curise_sw_check = False
            self.curise_set_first = 1
            self.prev_VSetDis = int(CS.VSetDis)
            set_speed_kph = CS.VSetDis
            if not CS.acc_active and self.prev_clu_CruiseSwState != CS.cruise_buttons:  # MODE ?�환.
                if CS.cruise_buttons == Buttons.GAP_DIST: 
                    self.cruise_set_mode += 1
                if self.cruise_set_mode > 4:
                    self.cruise_set_mode = 0
                self.prev_clu_CruiseSwState = CS.cruise_buttons

        if set_speed_kph < 30:
            set_speed_kph = 30

        self.cruise_set_speed_kph = set_speed_kph
        return self.cruise_set_mode, set_speed_kph


    @staticmethod
    def get_lead( sm ):
        dRel = 150
        vRel = 0
        
        lead = sm['radarState'].leadOne
        if lead.status:
            dRel = lead.dRel
            vRel = lead.vRel * CV.MS_TO_KPH + 0.5
        else:
            dRel = 150
            vRel = 0

        return dRel, vRel



    def get_tm_speed(self, CS, set_time, add_val, safety_dis=5):
        time = int(set_time)

        delta_speed = CS.VSetDis - CS.clu_Vanz
        set_speed = int(CS.VSetDis) + add_val
        
        if add_val > 0:  # inc
            if delta_speed > safety_dis:
                time = 100
        else:
            if delta_speed < -safety_dis:
                time = 100

        return time, set_speed

    # returns a 
    def update_lead(self, c, can_strings):
        raise NotImplementedError

    def update_curv(self, CS, sm, model_speed):
        raise NotImplementedError

    def update_log(self, CS, set_speed, target_set_speed, long_wait_cmd ):
        str3 = 'SET={:3.0f} DST={:3.0f} VSD={:.0f} DA={:.0f}/{:.0f}/{:.0f} DG={} DO={:.0f}'.format(
            set_speed, target_set_speed, CS.VSetDis, CS.driverAcc_time, long_wait_cmd, self.long_curv_timer, self.seq_step_debug, CS.driverOverride )
        str4 = ' CS={:.1f}/{:.1f} '.format(  CS.lead_distance, CS.lead_objspd )
        str5 = str3 +  str4
        trace1.printf2( str5 )

    def lead_control(self, CS, sm, CC ):
        dRel = CC.dRel
        vRel = CC.vRel
        active_time = 10
        btn_type = Buttons.NONE
        #lead_1 = sm['radarState'].leadOne
        long_wait_cmd = 500
        set_speed = self.cruise_set_speed_kph

        if self.long_curv_timer < 600:
            self.long_curv_timer += 1


        # ?�행 차량 거리?��?
        lead_wait_cmd, lead_set_speed = self.update_lead( CS,  dRel, vRel )  

        # 커브 감속.
        model_speed = CC.model_speed   #cal_model_speed( CS.out.vEgo )
        curv_wait_cmd, curv_set_speed = self.update_curv(CS, sm, model_speed)

        if curv_wait_cmd != 0:
            if lead_set_speed > curv_set_speed:
                set_speed = curv_set_speed
                long_wait_cmd = curv_wait_cmd
            else:
                set_speed = lead_set_speed
                long_wait_cmd = lead_wait_cmd
        else:
            set_speed = lead_set_speed
            long_wait_cmd = lead_wait_cmd

        if set_speed > self.cruise_set_speed_kph:
            set_speed = self.cruise_set_speed_kph
        elif set_speed < 30:
            set_speed = 30

        # control process
        target_set_speed = set_speed
        delta = int(set_speed) - int(CS.VSetDis)
        dec_step_cmd = 1


        if self.long_curv_timer < long_wait_cmd:
            pass
        elif CS.driverOverride == 1:  # acc
            if self.cruise_set_speed_kph > CS.clu_Vanz:
                delta = int(CS.clu_Vanz) - int(CS.VSetDis)
                if delta > 1:
                    set_speed = CS.clu_Vanz
                    btn_type = Buttons.SET_DECEL
        elif delta <= -1:
            set_speed = CS.VSetDis - dec_step_cmd
            btn_type = Buttons.SET_DECEL
            self.long_curv_timer = 0
        elif delta >= 1 and (model_speed > 200 or CS.clu_Vanz < 70):
            set_speed = CS.VSetDis + dec_step_cmd
            btn_type = Buttons.RES_ACCEL
            self.long_curv_timer = 0            
            if set_speed > self.cruise_set_speed_kph:
                set_speed = self.cruise_set_speed_kph
        if self.cruise_set_mode == 0:
            btn_type = Buttons.NONE


        self.update_log( CS, set_speed, target_set_speed, long_wait_cmd )


        return btn_type, set_speed, active_time



    def update(self, CS, sm, CC ):
        self.cruise_set_mode = CS.out.cruiseState.modeSel
        self.cruise_set_speed_kph = CS.out.cruiseState.speed * CV.MS_TO_KPH
        if CS.driverOverride == 2 or not CS.acc_active or CS.cruise_buttons == Buttons.RES_ACCEL or CS.cruise_buttons == Buttons.SET_DECEL:
            self.resume_cnt = 0
            self.btn_type = Buttons.NONE
            self.wait_timer2 = 10
            self.active_timer2 = 0
        elif self.wait_timer2:
            self.wait_timer2 -= 1
        else:
            btn_type, clu_speed, active_time = self.lead_control( CS, sm, CC )   # speed controller spdcontroller.py

            if CS.clu_Vanz < 20:
                self.btn_type = Buttons.NONE
            elif self.btn_type != Buttons.NONE:
                pass
            elif btn_type != Buttons.NONE:
                self.resume_cnt = 0
                self.active_timer2 = 0
                self.btn_type = btn_type
                self.sc_clu_speed = clu_speed                
                self.active_time = max( 5, active_time )

            if self.btn_type != Buttons.NONE:
                self.active_timer2 += 1
                if self.active_timer2 > self.active_time:
                    self.wait_timer2 = 5
                    self.resume_cnt = 0
                    self.active_timer2 = 0
                    self.btn_type = Buttons.NONE          
                else:
                    return 1
        return  0   
