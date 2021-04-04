from cereal import car, log
from common.numpy_fast import clip, interp
from selfdrive.controls.lib.pid import LongPIDController
from selfdrive.controls.lib.dynamic_gas import DynamicGas
from common.op_params import opParams
from selfdrive.config import Conversions as CV
from common.params import Params

import common.log as trace1
import common.CTime1000 as tm

LongCtrlState = log.ControlsState.LongControlState

STOPPING_EGO_SPEED = 0.5
STOPPING_TARGET_SPEED_OFFSET = 0.01
STARTING_TARGET_SPEED = 0.5
BRAKE_THRESHOLD_TO_PID = 0.2

BRAKE_STOPPING_TARGET = 0.5  # apply at least this amount of brake to maintain the vehicle stationary

RATE = 100.0


def long_control_state_trans(active, long_control_state, v_ego, v_target, v_pid,
                             output_gb, brake_pressed, cruise_standstill, stop, gas_pressed, min_speed_can):
  """Update longitudinal control state machine"""
  stopping_target_speed = min_speed_can + STOPPING_TARGET_SPEED_OFFSET
  stopping_condition = stop or (v_ego < 2.0 and cruise_standstill) or \
                       (v_ego < STOPPING_EGO_SPEED and \
                        ((v_pid < stopping_target_speed and v_target < stopping_target_speed) or
                        brake_pressed))

  starting_condition = v_target > STARTING_TARGET_SPEED and not cruise_standstill and (int(Params().get('OpkrAutoResume')) == 1 or gas_pressed)

  if not active:
    long_control_state = LongCtrlState.off

  else:
    if long_control_state == LongCtrlState.off:
      if active:
        long_control_state = LongCtrlState.pid

    elif long_control_state == LongCtrlState.pid:
      if stopping_condition:
        long_control_state = LongCtrlState.stopping

    elif long_control_state == LongCtrlState.stopping:
      if starting_condition:
        long_control_state = LongCtrlState.starting

    elif long_control_state == LongCtrlState.starting:
      if stopping_condition:
        long_control_state = LongCtrlState.stopping
      elif output_gb >= -BRAKE_THRESHOLD_TO_PID:
        long_control_state = LongCtrlState.pid

  return long_control_state


class LongControl():
  def __init__(self, CP, compute_gb, candidate):
    self.long_control_state = LongCtrlState.off  # initialized to off
    kdBP = [0., 16., 35.]
    kdV = [0.08, 1.215, 2.51]

    self.pid = LongPIDController((CP.longitudinalTuning.kpBP, CP.longitudinalTuning.kpV),
                                 (CP.longitudinalTuning.kiBP, CP.longitudinalTuning.kiV),
                                 (kdBP, kdV),
                                 rate=RATE,
                                 sat_limit=0.8,
                                 convert=compute_gb)

    self.v_pid = 0.0
    self.last_output_gb = 0.0
    self.long_stat = ""

    self.op_params = opParams()
    self.dynamic_gas = DynamicGas(CP, candidate)
    self.stopped = False

  def reset(self, v_pid):
    """Reset PID controller and change setpoint"""
    self.pid.reset()
    self.v_pid = v_pid

  def update(self, active, CS, v_target, v_target_future, a_target_raw, a_target, CP, hasLead, radarState, longitudinalPlanSource, extras):
    """Update longitudinal control. This updates the state machine and runs a PID loop"""
    # Actuation limits
    gas_max = interp(CS.vEgo, CP.gasMaxBP, CP.gasMaxV)
    brake_max = interp(CS.vEgo, CP.brakeMaxBP, CP.brakeMaxV)

    if self.op_params.get('dynamic_gas'):
      gas_max = self.dynamic_gas.update(CS, extras)

    # Update state machine
    output_gb = self.last_output_gb

    if radarState is None:
      dRel = 200
      vRel = 0
    else:
      dRel = radarState.leadOne.dRel
      vRel = radarState.leadOne.vRel
    if hasLead:
      stop = True if (dRel < 4.2 and radarState.leadOne.status) else False
    else:
      stop = False
    self.long_control_state = long_control_state_trans(active, self.long_control_state, CS.vEgo,
                                                       v_target_future, self.v_pid, output_gb,
                                                       CS.brakePressed, CS.cruiseState.standstill, stop, CS.gasPressed, CP.minSpeedCan)

    v_ego_pid = max(CS.vEgo, CP.minSpeedCan)  # Without this we get jumps, CAN bus reports 0 when speed < 0.3
    if self.long_control_state == LongCtrlState.off or (CS.brakePressed or CS.gasPressed):
      self.v_pid = v_ego_pid
      self.pid.reset()
      output_gb = 0.

    # tracking objects and driving
    elif self.long_control_state == LongCtrlState.pid:
      self.v_pid = v_target
      self.pid.pos_limit = gas_max
      self.pid.neg_limit = - brake_max
      afactor = 1
      vfactor = 1
      dfactor = 1
      dvfactor = 1
      gasadd = 1

      # Toyota starts braking more when it thinks you want to stop
      # Freeze the integrator so we don't accelerate to compensate, and don't allow positive acceleration
      prevent_overshoot = not CP.stoppingControl and CS.vEgo < 1.5 and v_target_future < 0.7
      deadzone = interp(v_ego_pid, CP.longitudinalTuning.deadzoneBP, CP.longitudinalTuning.deadzoneV)
      
      # added by opkr to control different tune when vehicle start from stop
      output_gb = self.pid.update(self.v_pid, v_ego_pid, speed=v_ego_pid, deadzone=deadzone, feedforward=a_target, freeze_integrator=prevent_overshoot)

      # added by opkr
      afactor = interp(CS.vEgo,[0,1,2,3,4,8,12,16,20], [4.5,4.2,3.65,3.375,3.1,2.3,2.1,2,2])
      vfactor = interp(dRel,[1,30,50], [15,7,4])
      dfactor = interp(dRel,[4,10], [1.6,1])
      dvfactor = interp(((CS.vEgo*3.6)/(max(3,dRel))),[1,2,3], [1,3,5])
      gasadd = interp((vRel*3.6),[1,10], [1,2.3])

      if abs(output_gb) < abs(a_target_raw)/afactor and a_target_raw < 0 and dRel >= 4.2:
        output_gb = (-abs(a_target_raw)/afactor)*dfactor
      elif output_gb > 0 and a_target_raw < 0 and dRel >= 4.2:
        output_gb = output_gb/vfactor
      #elif output_gb > 0 and a_target_raw > 0 and 25 > dRel >= 4.2 and (CS.vEgo*3.6) < 35 and self.stopped and hasLead:
      #  output_gb = output_gb*gasadd
      elif output_gb > 0 and a_target_raw > 0 and dRel >= 4.2 and (CS.vEgo*3.6) < 65:
        output_gb = output_gb/dvfactor
      
      if vRel*3.6 < 5 and dRel >= 8 and self.stopped:
        self.stopped = False
      elif not hasLead:
        self.stopped = False

      if prevent_overshoot or CS.brakeHold:
        output_gb = min(output_gb, 0.0)

    # Intention is to stop, switch to a different brake control until we stop
    elif self.long_control_state == LongCtrlState.stopping:
      # Keep applying brakes until the car is stopped
      factor = 1
      if hasLead:
        factor = interp(dRel,[2.0,4.2,5.0,6.0,7.0,8.0], [2.5,1,0.7,0.5,0.3,0.0])
      if not CS.standstill or output_gb > -BRAKE_STOPPING_TARGET:
        output_gb -= CP.stoppingBrakeRate / RATE * factor
      elif CS.cruiseState.standstill and output_gb < -BRAKE_STOPPING_TARGET:
        output_gb += CP.stoppingBrakeRate / RATE
      output_gb = clip(output_gb, -brake_max, gas_max)

      self.stopped = True
      self.reset(CS.vEgo)

    # Intention is to move again, release brake fast before handing control to PID
    elif self.long_control_state == LongCtrlState.starting:
      factor = 1
      if hasLead:
        factor = interp(dRel,[0.0,2.0,3.0,4.2,5.0], [0.0,0.5,1,500.0,1500.0])
      if output_gb < -0.2:
        output_gb += CP.startingBrakeRate / RATE * factor
      self.reset(CS.vEgo)

    self.last_output_gb = output_gb
    final_gas = clip(output_gb, 0., gas_max)
    final_brake = -clip(output_gb, -brake_max, 0.)


    if self.long_control_state == LongCtrlState.stopping:
      self.long_stat = "STP"
    elif self.long_control_state == LongCtrlState.starting:
      self.long_stat = "STR"
    elif self.long_control_state == LongCtrlState.pid:
      self.long_stat = "PID"
    elif self.long_control_state == LongCtrlState.off:
      self.long_stat = "OFF"
    else:
      self.long_stat = "---"

    str_log3 = 'LS={:s}  GS={:01.2f}/{:01.2f}  BK={:01.2f}/{:01.2f}  GB={:+04.2f}  TG=P:{:05.2f}/F:{:05.2f}/A:{:04.2f}/AR:{:04.2f}  GS={}'.format(self.long_stat, final_gas, gas_max, abs(final_brake), abs(brake_max), output_gb, abs(self.v_pid), abs(v_target_future), a_target, a_target_raw, CS.gasPressed)
    trace1.printf2('{}'.format(str_log3))

    return float(final_gas), float(final_brake)