import json
import os


json_file_name = '/data/openpilot/atom_3.json'

class conf_kegman():
  def __init__(self, CP=None):
    self.config = None
    self.init = { 
        "cv_KPH": [30,50,100],
        "cv_BPV": [[255],[60,255],[80,255]],
        "cv_sMaxV": [[384],[384,384],[384,384]],
        "cv_sdDNV": [[1],[3,1],[5,1]],
        "cv_sdUPV": [[1],[2,1],[3,1]],
        "cv_steerRatioV": [[13.8],[16.0,15.25],[17.5,15.25]],
        "cv_ActuatorDelayV": [[0.0],[0.02,0.1],[0.05,0.2]]     
         }


  def data_check(self, name, value ):
    if name not in self.config:
        self.config.update({name:value})
        self.element_updated = True


  def read_config(self):
    self.element_updated = False

    if os.path.isfile( json_file_name ):
      with open( json_file_name, 'r') as f:
        str_kegman = f.read()
        print( str_kegman )
        self.config = json.loads(str_kegman)

      for name in self.init:
        self.data_check( name, self.init[name] )

      if self.element_updated:
        print("updated")
        self.write_config(self.config)

    else:
      self.config = self.init      
      self.write_config(self.config)

    return self.config

  def write_config(self, config):
    try:
      with open( json_file_name, 'w') as f:
        json.dump(self.config, f, indent=2, sort_keys=True)
        os.chmod( json_file_name, 0o764)
    except IOError:
      os.mkdir('/data')
      with open( json_file_name, 'w') as f:
        json.dump(self.config, f, indent=2, sort_keys=True)
        os.chmod( json_file_name, 0o764)
