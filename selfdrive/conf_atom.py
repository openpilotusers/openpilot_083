
from  selfdrive.conf_kegman import conf_kegman

class ConfAtom():
  def __init__(self, CP=None):
    self.kegman = conf_kegman()


    self.cv_steerRatioV = [[13.85]]
    self.cv_ActuatorDelayV = [[0.1]]
    self.cv_KPH    = [0.]   # Speed  kph
    self.cv_BPV    = [[200, 255]]  # CV
    self.cv_sMaxV  = [[384, 255]]
    self.cv_sdUPV  = [[3,1]]
    self.cv_sdDNV  = [[7,1]]

    self.read_tune()


  def read_tune(self):
    conf = self.kegman.read_config()

    self.cv_KPH  = conf['cv_KPH']
    self.cv_BPV  = conf['cv_BPV']
    self.cv_sMaxV  = conf['cv_sMaxV']
    self.cv_sdUPV  = conf['cv_sdUPV']
    self.cv_sdDNV = conf['cv_sdDNV']
    self.cv_steerRatioV  = conf['cv_steerRatioV']
    self.cv_ActuatorDelayV = conf['cv_ActuatorDelayV'] 

