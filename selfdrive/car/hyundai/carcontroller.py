from cereal import car, log
from common.realtime import DT_CTRL
from selfdrive.car import apply_std_steer_torque_limits
from selfdrive.car.hyundai.hyundaican import create_lkas11, create_clu11, create_lfahda_mfc, create_mdps12
from selfdrive.car.hyundai.values import Buttons, CarControllerParams, CAR, FEATURES
from opendbc.can.packer import CANPacker
from selfdrive.config import Conversions as CV
from common.numpy_fast import interp

from selfdrive.controls.lib.lateral_planner import LANE_CHANGE_SPEED_MIN
# speed controller
from selfdrive.car.hyundai.spdcontroller  import SpdController
from selfdrive.car.hyundai.spdctrl  import Spdctrl
from selfdrive.car.hyundai.spdctrlRelaxed  import SpdctrlRelaxed

from common.params import Params
import common.log as trace1
import common.CTime1000 as tm
from random import randint

VisualAlert = car.CarControl.HUDControl.VisualAlert


def process_hud_alert(enabled, fingerprint, visual_alert, left_lane,
                      right_lane, left_lane_depart, right_lane_depart, button_on):
  sys_warning = (visual_alert == VisualAlert.steerRequired)

  # initialize to no line visible
  sys_state = 1
  if not button_on:
    lane_visible = 0
  if left_lane and right_lane or sys_warning:  #HUD alert only display when LKAS status is active
    if enabled or sys_warning:
      sys_state = 3
    else:
      sys_state = 4
  elif left_lane:
    sys_state = 5
  elif right_lane:
    sys_state = 6

  # initialize to no warnings
  left_lane_warning = 0
  right_lane_warning = 0
  #if left_lane_depart:
  #  left_lane_warning = 1 if fingerprint in [CAR.GENESIS, CAR.GENESIS_G70, CAR.GENESIS_G80,
  #                                           CAR.GENESIS_G90, CAR.GENESIS_G90_L] else 2
  #if right_lane_depart:
  #  right_lane_warning = 1 if fingerprint in [CAR.GENESIS, CAR.GENESIS_G70, CAR.GENESIS_G80,
  #                                            CAR.GENESIS_G90, CAR.GENESIS_G90_L] else 2

  return sys_warning, sys_state, left_lane_warning, right_lane_warning


class CarController():
  def __init__(self, dbc_name, CP, VM):
    self.packer = CANPacker(dbc_name)

    self.apply_steer_last = 0
    self.car_fingerprint = CP.carFingerprint
    self.steer_rate_limited = False
    self.last_resume_frame = 0

    self.last_lead_distance = 0
    self.resume_cnt = 0
    self.lkas11_cnt = 0 
    self.last_lead_distance = 0
    self.resume_wait_timer = 0
    self.lanechange_manual_timer = 0
    self.emergency_manual_timer = 0
    self.driver_steering_torque_above = False
    self.driver_steering_torque_above_timer = 100
    self.mode_change_timer = 0
    self.need_brake = False
    self.need_brake_timer = 0

    self.steer_mode = 0
    self.mdps_status = 0
    self.lkas_switch = 0
    self.leadcar_status = 0

    self.dRel = 0
    self.vRel = 0

    self.params = Params()

    self.mode_change_switch = int(self.params.get("CruiseStatemodeSelInit", encoding='utf8'))
    self.opkr_variablecruise = self.params.get('OpkrVariableCruise') == b'1'
    self.opkr_autoresume = self.params.get('OpkrAutoResume') == b'1'
    self.opkr_turnsteeringdisable = self.params.get('OpkrTurnSteeringDisable') == b'1'
    self.opkr_maxanglelimit = float(int(self.params.get("OpkrMaxAngleLimit", encoding='utf8')))

    self.timer1 = tm.CTime1000("time")

    if int(self.params.get("OpkrVariableCruiseProfile", encoding='utf8')) == 0:
      self.SC = Spdctrl()
    elif int(self.params.get("OpkrVariableCruiseProfile", encoding='utf8')) == 1:
      self.SC = SpdctrlRelaxed()
    else:
      self.SC = Spdctrl()

    self.model_speed = 0
    self.curve_speed = 0

    self.dRel = 0
    self.vRel = 0
    self.dRel2 = 0
    self.vRel2 = 0

    self.cruise_gap = 0.0
    self.cruise_gap_prev = 0
    self.cruise_gap_set_init = 0
    self.cruise_gap_switch_timer = 0
    self.standstill_fault_reduce_timer = 0
    self.cruise_gap_prev2 = 0
    self.cruise_gap_switch_timer2 = 0
    self.cruise_gap_switch_timer3 = 0
    self.standstill_status = 0
    self.standstill_status_timer = 0
    self.res_switch_timer = 0

    self.lkas_button_on = True

    self.steerMax = 0
    self.steerDeltaUp = 0
    self.steerDeltaDown = 0

    self.steerMaxBase = int(self.params.get("SteerMaxBaseAdj", encoding='utf8'))
    self.steerDeltaUpBase = int(self.params.get("SteerDeltaUpBaseAdj", encoding='utf8'))
    self.steerDeltaDownBase = int(self.params.get("SteerDeltaDownBaseAdj", encoding='utf8'))

    self.model_speed_range = [30, 90, 255]
    self.steerMax_range = [CarControllerParams.STEER_MAX, self.steerMaxBase, self.steerMaxBase]
    self.steerDeltaUp_range = [CarControllerParams.STEER_DELTA_UP, self.steerDeltaUpBase, self.steerDeltaUpBase]
    self.steerDeltaDown_range = [CarControllerParams.STEER_DELTA_DOWN, self.steerDeltaDownBase, self.steerDeltaDownBase]

    self.variable_steer_max = self.params.get('OpkrVariableSteerMax') == b'1'
    self.variable_steer_delta = self.params.get('OpkrVariableSteerDelta') == b'1'

    if CP.lateralTuning.which() == 'pid':
      self.str_log2 = 'T={:0.2f}/{:0.3f}/{:0.2f}/{:0.5f}'.format(CP.lateralTuning.pid.kpV[1], CP.lateralTuning.pid.kiV[1], CP.lateralTuning.pid.kdV[0], CP.lateralTuning.pid.kf)
    elif CP.lateralTuning.which() == 'indi':
      self.str_log2 = 'T={:03.1f}/{:03.1f}/{:03.1f}/{:03.1f}'.format(CP.lateralTuning.indi.innerLoopGainV[1], CP.lateralTuning.indi.outerLoopGainV[1], CP.lateralTuning.indi.timeConstantV[1], CP.lateralTuning.indi.actuatorEffectivenessV[1])
    elif CP.lateralTuning.which() == 'lqr':
      self.str_log2 = 'T={:04.0f}/{:05.3f}/{:06.4f}'.format(CP.lateralTuning.lqr.scale, CP.lateralTuning.lqr.ki, CP.lateralTuning.lqr.dcGain)

    self.p = CarControllerParams
  def update(self, enabled, CS, frame, CC, actuators, pcm_cancel_cmd, visual_alert,
             left_lane, right_lane, left_lane_depart, right_lane_depart, sm):

    # *** compute control surfaces ***
    param = self.p

    #self.model_speed = 255 - self.SC.calc_va(sm, CS.out.vEgo)
    #atom model_speed
    #self.model_speed = self.SC.cal_model_speed(sm, CS.out.vEgo)
    #self.curve_speed = self.SC.cal_curve_speed(sm, CS.out.vEgo, frame)

    plan = sm['longitudinalPlan']
    self.dRel = int(plan.dRel1) #EON Lead
    self.vRel = int(plan.vRel1 * 3.6 + 0.5) #EON Lead
    self.dRel2 = int(plan.dRel2) #EON Lead
    self.vRel2 = int(plan.vRel2 * 3.6 + 0.5) #EON Lead

    self.accActive = CS.acc_active

    lateral_plan = sm['lateralPlan']
    self.outScale = lateral_plan.outputScale
    self.vCruiseSet = lateral_plan.vCruiseSet
    
    #self.model_speed = interp(abs(lateral_plan.vCurvature), [0.0002, 0.01], [255, 30])
    #Hoya
    self.model_speed = interp(abs(lateral_plan.vCurvature), [0.0, 0.0002, 0.00074, 0.0025, 0.008, 0.02], [255, 255, 130, 90, 60, 20])

    if CS.out.vEgo > 8:
      if self.variable_steer_max:
        self.steerMax = interp(int(abs(self.model_speed)), self.model_speed_range, self.steerMax_range)
      else:
        self.steerMax = self.steerMaxBase
      if self.variable_steer_delta:
        self.steerDeltaUp = interp(int(abs(self.model_speed)), self.model_speed_range, self.steerDeltaUp_range)
        self.steerDeltaDown = interp(int(abs(self.model_speed)), self.model_speed_range, self.steerDeltaDown_range)
      else:
        self.steerDeltaUp = self.steerDeltaUpBase
        self.steerDeltaDown = self.steerDeltaDownBase
    else:
      self.steerMax = self.steerMaxBase
      self.steerDeltaUp = self.steerDeltaUpBase
      self.steerDeltaDown = self.steerDeltaDownBase

    param.STEER_MAX = min(CarControllerParams.STEER_MAX, self.steerMax) # variable steermax
    param.STEER_DELTA_UP = min(CarControllerParams.STEER_DELTA_UP, self.steerDeltaUp) # variable deltaUp
    param.STEER_DELTA_DOWN = min(CarControllerParams.STEER_DELTA_DOWN, self.steerDeltaDown) # variable deltaDown

    # Steering Torque
    if 0 <= self.driver_steering_torque_above_timer < 100:
      new_steer = int(round(actuators.steer * self.steerMax * (self.driver_steering_torque_above_timer / 100)))
    else:
      new_steer = int(round(actuators.steer * self.steerMax))
    apply_steer = apply_std_steer_torque_limits(new_steer, self.apply_steer_last, CS.out.steeringTorque, param)
    self.steer_rate_limited = new_steer != apply_steer

    # disable if steer angle reach 90 deg, otherwise mdps fault in some models
    if self.opkr_maxanglelimit >= 90:
      lkas_active = enabled and abs(CS.out.steeringAngleDeg) < self.opkr_maxanglelimit
    else:
      lkas_active = enabled

    if (( CS.out.leftBlinker and not CS.out.rightBlinker) or ( CS.out.rightBlinker and not CS.out.leftBlinker)) and CS.out.vEgo < LANE_CHANGE_SPEED_MIN and self.opkr_turnsteeringdisable:
      self.lanechange_manual_timer = 50
    if CS.out.leftBlinker and CS.out.rightBlinker:
      self.emergency_manual_timer = 50
    if self.lanechange_manual_timer:
      lkas_active = 0
    if self.lanechange_manual_timer > 0:
      self.lanechange_manual_timer -= 1
    if self.emergency_manual_timer > 0:
      self.emergency_manual_timer -= 1

    if abs(CS.out.steeringTorque) > 180 and CS.out.vEgo < LANE_CHANGE_SPEED_MIN:
      self.driver_steering_torque_above = True
    else:
      self.driver_steering_torque_above = False

    if self.driver_steering_torque_above == True:
      self.driver_steering_torque_above_timer -= 1
      if self.driver_steering_torque_above_timer <= 0:
        self.driver_steering_torque_above_timer = 0
    elif self.driver_steering_torque_above == False:
      self.driver_steering_torque_above_timer += 5
      if self.driver_steering_torque_above_timer >= 100:
        self.driver_steering_torque_above_timer = 100

    if not lkas_active:
      apply_steer = 0

    self.apply_steer_last = apply_steer
    if CS.acc_active and CS.lead_distance > 149 and self.dRel < ((CS.out.vEgo * CV.MS_TO_KPH)+5) < 100 and self.vRel < -5 and CS.out.vEgo > 7 and abs(lateral_plan.steerAngleDesireDeg) < 15:
      self.need_brake_timer += 1
      if self.need_brake_timer > 50:
        self.need_brake = True
    else:
      self.need_brake = False
      self.need_brake_timer = 0

    sys_warning, sys_state, left_lane_warning, right_lane_warning =\
      process_hud_alert(lkas_active, self.car_fingerprint, visual_alert,
                        left_lane, right_lane, left_lane_depart, right_lane_depart,
                        self.lkas_button_on)

    if CS.mdps_bus: # send clu11 to mdps if it is not on bus 0
      clu11_speed = CS.clu_Vanz
      enabled_speed = 55
      if clu11_speed > enabled_speed or not lkas_active:
        enabled_speed = clu11_speed

    if frame == 0: # initialize counts from last received count signals
      self.lkas11_cnt = CS.lkas11["CF_Lkas_MsgCount"] + 1
    self.lkas11_cnt %= 0x10

    can_sends = []
    can_sends.append(create_lkas11(self.packer, frame, self.car_fingerprint, apply_steer, lkas_active,
                                   CS.lkas11, sys_warning, sys_state, enabled, left_lane, right_lane,
                                   left_lane_warning, right_lane_warning, 0))

    if CS.mdps_bus: # send lkas11 bus 1 if mdps is on bus 1
      can_sends.append( create_clu11(self.packer, frame, CS.clu11, Buttons.NONE, enabled_speed, CS.mdps_bus) )
      can_sends.append(create_lkas11(self.packer, frame, self.car_fingerprint, apply_steer, lkas_active,
                                   CS.lkas11, sys_warning, sys_state, enabled, left_lane, right_lane,
                                   left_lane_warning, right_lane_warning, 1))

    if CS.mdps_bus: # send mdps12 to LKAS to prevent LKAS error
      can_sends.append(create_mdps12(self.packer, frame, CS.mdps12))

    str_log1 = 'CV={:03.0f}  TQ={:03.0f}  R={:03.0f}  ST={:03.0f}/{:01.0f}/{:01.0f}'.format(abs(self.model_speed), abs(new_steer), self.timer1.sampleTime(), self.steerMax, self.steerDeltaUp, self.steerDeltaDown)
    trace1.printf1('{}  {}'.format(str_log1, self.str_log2))

    if CS.out.cruiseState.modeSel == 0 and self.mode_change_switch == 3:
      self.mode_change_timer = 50
      self.mode_change_switch = 0
    elif CS.out.cruiseState.modeSel == 1 and self.mode_change_switch == 0:
      self.mode_change_timer = 50
      self.mode_change_switch = 1
    elif CS.out.cruiseState.modeSel == 2 and self.mode_change_switch == 1:
      self.mode_change_timer = 50
      self.mode_change_switch = 2
    elif CS.out.cruiseState.modeSel == 3 and self.mode_change_switch == 2:
      self.mode_change_timer = 50
      self.mode_change_switch = 3
    if self.mode_change_timer > 0:
      self.mode_change_timer -= 1

    run_speed_ctrl = self.opkr_variablecruise and CS.acc_active and (CS.out.cruiseState.modeSel == 1 or CS.out.cruiseState.modeSel == 2 or CS.out.cruiseState.modeSel == 3)

    if not run_speed_ctrl:
      str_log2 = 'MODE={}  MDPS={}  LKAS={}  CSG={:1.0f}  LEAD={:1.1f}'.format(CS.out.cruiseState.modeSel, CS.out.steerWarning, CS.lkas_button_on, CS.cruiseGapSet, CS.lead_distance)
      trace1.printf2( '{}'.format( str_log2 ) )


    if pcm_cancel_cmd:
      can_sends.append(create_clu11(self.packer, frame, CS.clu11, Buttons.CANCEL))
    if CS.out.cruiseState.standstill:
      self.standstill_status = 1
      if self.opkr_autoresume:
        # run only first time when the car stopped
        if self.last_lead_distance == 0:
          # get the lead distance from the Radar
          self.last_lead_distance = CS.lead_distance
          self.resume_cnt = 0
          self.res_switch_timer = 0
          self.standstill_fault_reduce_timer += 1
        elif self.res_switch_timer > 0:
          self.res_switch_timer -= 1
          self.standstill_fault_reduce_timer += 1
        # at least 1 sec delay after entering the standstill
        elif 100 < self.standstill_fault_reduce_timer and CS.lead_distance != self.last_lead_distance:
          can_sends.append(create_clu11(self.packer, self.resume_cnt, CS.clu11, Buttons.RES_ACCEL))
          self.resume_cnt += 1
          if self.resume_cnt > 5:
            self.resume_cnt = 0
            self.res_switch_timer = randint(10, 15)
          self.standstill_fault_reduce_timer += 1
        # gap save
        elif 160 < self.standstill_fault_reduce_timer and self.cruise_gap_prev == 0 and self.opkr_autoresume: 
          self.cruise_gap_prev = CS.cruiseGapSet
          self.cruise_gap_set_init = 1
        # gap adjust to 1 for fast start
        elif 160 < self.standstill_fault_reduce_timer and CS.cruiseGapSet != 1.0 and self.opkr_autoresume:
          self.cruise_gap_switch_timer += 1
          if self.cruise_gap_switch_timer > 100:
            can_sends.append(create_clu11(self.packer, self.resume_cnt, CS.clu11, Buttons.GAP_DIST))
            self.cruise_gap_switch_timer = 0
        elif self.opkr_autoresume:
          self.standstill_fault_reduce_timer += 1
    # reset lead distnce after the car starts moving
    elif self.last_lead_distance != 0:
      self.last_lead_distance = 0
    elif run_speed_ctrl:
      is_sc_run = self.SC.update(CS, sm, self)
      if is_sc_run:
        can_sends.append(create_clu11(self.packer, self.resume_cnt, CS.clu11, self.SC.btn_type, self.SC.sc_clu_speed))
        self.resume_cnt += 1
      else:
        self.resume_cnt = 0
      # gap restore
      if self.dRel > 17 and self.vRel < 5 and self.cruise_gap_prev != CS.cruiseGapSet and self.cruise_gap_set_init == 1 and self.opkr_autoresume:
        self.cruise_gap_switch_timer += 1
        if self.cruise_gap_switch_timer > 50:
          can_sends.append(create_clu11(self.packer, frame, CS.scc_bus, CS.clu11, Buttons.GAP_DIST, clu11_speed))
          self.cruise_gap_switch_timer = 0
      elif self.cruise_gap_prev == CS.cruiseGapSet and self.opkr_autoresume:
        self.cruise_gap_set_init = 0
        self.cruise_gap_prev = 0
    
    if CS.out.brakeLights and CS.out.vEgo == 0 and not CS.acc_active:
      self.standstill_status_timer += 1
      if self.standstill_status_timer > 200:
        self.standstill_status = 1
        self.standstill_status_timer = 0
    if self.standstill_status == 1 and CS.out.vEgo > 1:
      self.standstill_status = 0
      self.standstill_fault_reduce_timer = 0
      self.v_set_dis_prev = 180
      self.last_resume_frame = frame
      self.res_switch_timer = 0
      self.resume_cnt = 0

    # 20 Hz LFA MFA message
    if frame % 5 == 0 and self.car_fingerprint in FEATURES["send_lfahda_mfa"]:
      can_sends.append(create_lfahda_mfc(self.packer, enabled))

   # counter inc
    self.lkas11_cnt += 1
    return can_sends
