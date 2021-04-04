#this was initiated by atom(conan)
#partially modified by opkr
import math
import numpy as np
from cereal import car, log
import cereal.messaging as messaging
from selfdrive.config import Conversions as CV
from selfdrive.controls.lib.speed_smoother import speed_smoother
from selfdrive.controls.lib.long_mpc import LongitudinalMpc
from selfdrive.controls.lib.lane_planner import TRAJECTORY_SIZE
from common.numpy_fast import clip, interp
import common.MoveAvg as moveavg1

MAX_SPEED = 255.0

class SpdController():
    def __init__(self, CP=None):
        self.long_control_state = 0  # initialized to off
        self.path_x = np.arange(192)

        self.v_model = 0
        self.a_model = 0
        self.v_cruise = 0
        self.a_cruise = 0

        self.l_poly = []
        self.r_poly = []
        self.movAvg = moveavg1.MoveAvg()

        self.old_model_speed = 0
        self.old_model_init = 0


    def reset(self):
        self.v_model = 0
        self.a_model = 0
        self.v_cruise = 0
        self.a_cruise = 0

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