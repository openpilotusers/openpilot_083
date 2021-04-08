#pragma once

#include <QTimer>
#include <QPushButton>
#include <QNetworkAccessManager>

#include "widgets/controls.hpp"
#include "selfdrive/hardware/hw.h"

// SSH enable toggle
class SshToggle : public ToggleControl {
  Q_OBJECT

public:
  SshToggle() : ToggleControl("SSH 접속 사용", "", "", Hardware::get_ssh_enabled()) {
    QObject::connect(this, &SshToggle::toggleFlipped, [=](bool state) {
      Hardware::set_ssh_enabled(state);
    });
  }
};

class SshLegacyToggle : public ToggleControl {
  Q_OBJECT

public:
  SshLegacyToggle() : ToggleControl("기존 공개KEY 사용", "SSH 접속시 기존 공개KEY(0.8.2이하)를 사용합니다.", "", Params().read_db_bool("OpkrSSHLegacy")) {
    QObject::connect(this, &SshLegacyToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("OpkrSSHLegacy", &value, 1);
    });
  }
};

class GetoffAlertToggle : public ToggleControl {
  Q_OBJECT

public:
  GetoffAlertToggle() : ToggleControl("운행종료시 이온탈착 알림 사용", "운행종료 후 이온을 분리하라는 알림을 보냅니다.", "../assets/offroad/icon_shell.png", Params().read_db_bool("OpkrEnableGetoffAlert")) {
    QObject::connect(this, &GetoffAlertToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("OpkrEnableGetoffAlert", &value, 1);
    });
  }
};

class AutoResumeToggle : public ToggleControl {
  Q_OBJECT

public:
  AutoResumeToggle() : ToggleControl("자동출발 기능 사용", "SCC 사용중 정차시 자동출발 기능을 사용합니다.", "../assets/offroad/icon_shell.png", Params().read_db_bool("OpkrAutoResume")) {
    QObject::connect(this, &AutoResumeToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("OpkrAutoResume", &value, 1);
    });
  }
};

class VariableCruiseToggle : public ToggleControl {
  Q_OBJECT

public:
  VariableCruiseToggle() : ToggleControl("가변 크루즈 사용", "SCC 사용중 크루즈 버튼을 이용하여 가감속을 보조합니다.", "../assets/offroad/icon_shell.png", Params().read_db_bool("OpkrVariableCruise")) {
    QObject::connect(this, &VariableCruiseToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("OpkrVariableCruise", &value, 1);
    });
  }
};

class BatteryChargingControlToggle : public ToggleControl {
  Q_OBJECT

public:
  BatteryChargingControlToggle() : ToggleControl("배터리 충전 제어기능 사용", "배터리 충전제어 기능을 사용합니다.", "../assets/offroad/icon_shell.png", Params().read_db_bool("OpkrBatteryChargingControl")) {
    QObject::connect(this, &BatteryChargingControlToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("OpkrBatteryChargingControl", &value, 1);
    });
  }
};

class BlindSpotDetectToggle : public ToggleControl {
  Q_OBJECT

public:
  BlindSpotDetectToggle() : ToggleControl("후측방 감지 아이콘 표시", "후측방에 차가 감지되면 화면에 아이콘을 띄웁니다.", "../assets/offroad/icon_shell.png", Params().read_db_bool("OpkrBlindSpotDetect")) {
    QObject::connect(this, &BlindSpotDetectToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("OpkrBlindSpotDetect", &value, 1);
    });
  }
};

class AutoScreenDimmingToggle : public ToggleControl {
  Q_OBJECT

public:
  AutoScreenDimmingToggle() : ToggleControl("주행화면 Dimming 제어", "주행시 최소한의 밝기를 유지하여 배터리 소모량 및 발열을 줄이며, 이벤트 발생 시 밝기를 높여 일시적으로 가시성을 확보합니다.", "../assets/offroad/icon_shell.png", Params().read_db_bool("OpkrAutoScreenDimming")) {
    QObject::connect(this, &AutoScreenDimmingToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("OpkrAutoScreenDimming", &value, 1);
    });
  }
};

class LiveSteerRatioToggle : public ToggleControl {
  Q_OBJECT

public:
  LiveSteerRatioToggle() : ToggleControl("Live SteerRatio 사용", "가변/고정 SR 대신 Live SteerRatio를 사용합니다.", "../assets/offroad/icon_shell.png", Params().read_db_bool("OpkrLiveSteerRatio")) {
    QObject::connect(this, &LiveSteerRatioToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("OpkrLiveSteerRatio", &value, 1);
    });
  }
};

class VariableSteerMaxToggle : public ToggleControl {
  Q_OBJECT

public:
  VariableSteerMaxToggle() : ToggleControl("가변 SteerMax 사용", "곡률에 따른 가변 SteerMax을 사용합니다.", "../assets/offroad/icon_shell.png", Params().read_db_bool("OpkrVariableSteerMax")) {
    QObject::connect(this, &VariableSteerMaxToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("OpkrVariableSteerMax", &value, 1);
    });
  }
};

class VariableSteerDeltaToggle : public ToggleControl {
  Q_OBJECT

public:
  VariableSteerDeltaToggle() : ToggleControl("가변 SteerDelta 사용", "곡률에 따른 가변 SteerDelta를 사용합니다.( DeltaUp ~ 5까지 변화, DeltaDown ~ 10까지 변화", "../assets/offroad/icon_shell.png", Params().read_db_bool("OpkrVariableSteerDelta")) {
    QObject::connect(this, &VariableSteerDeltaToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("OpkrVariableSteerDelta", &value, 1);
    });
  }
};

class LiveTuneToggle : public ToggleControl {
  Q_OBJECT

public:
  LiveTuneToggle() : ToggleControl("라이브 튜너 사용", "이 옵션을 켜면 제어값이 실시간으로 적용됩니다.", "../assets/offroad/icon_shell.png", Params().read_db_bool("OpkrLiveTune")) {
    QObject::connect(this, &LiveTuneToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("OpkrLiveTune", &value, 1);
    });
  }
};

class ShaneFeedForward : public ToggleControl {
  Q_OBJECT

public:
  ShaneFeedForward() : ToggleControl("Shane FeedForward 사용", "Shane의 FeedForward를 사용합니다. 조향각에 따라 직선주로에서는 토크를 낮추고, 곡선주로에서는 동적으로 조정합니다.", "../assets/offroad/icon_shell.png", Params().read_db_bool("ShaneFeedForward")) {
    QObject::connect(this, &ShaneFeedForward::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("ShaneFeedForward", &value, 1);
    });
  }
};

class DrivingRecordToggle : public ToggleControl {
  Q_OBJECT

public:
  DrivingRecordToggle() : ToggleControl("자동 화면녹화 기능 사용", "운전 중 화면 녹화/중지를 자동으로 수행합니다. 출발 후 녹화가 시작되며 차량이 정지하면 녹화가 종료됩니다.", "../assets/offroad/icon_shell.png", Params().read_db_bool("OpkrDrivingRecord")) {
    QObject::connect(this, &DrivingRecordToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("OpkrDrivingRecord", &value, 1);
    });
  }
};

class TurnSteeringDisableToggle : public ToggleControl {
  Q_OBJECT

public:
  TurnSteeringDisableToggle() : ToggleControl("턴시그널 사용시 조향해제 사용", "차선변경속도 이하로 주행할 때 턴시그널을 사용시 자동조향을 일시해제 합니다.", "../assets/offroad/icon_shell.png", Params().read_db_bool("OpkrTurnSteeringDisable")) {
    QObject::connect(this, &TurnSteeringDisableToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("OpkrTurnSteeringDisable", &value, 1);
    });
  }
};

class HotspotOnBootToggle : public ToggleControl {
  Q_OBJECT

public:
  HotspotOnBootToggle() : ToggleControl("부팅시 핫스팟 자동실행", "부팅 후 핫스팟을 자동으로 실행합니다.", "../assets/offroad/icon_shell.png", Params().read_db_bool("OpkrHotspotOnBoot")) {
    QObject::connect(this, &HotspotOnBootToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("OpkrHotspotOnBoot", &value, 1);
    });
  }
};

class CruiseOverMaxSpeedToggle : public ToggleControl {
  Q_OBJECT

public:
  CruiseOverMaxSpeedToggle() : ToggleControl("설정속도를 초과속도에 동기화", "현재속도가 설정속도를 넘어설 경우 설정속도를 현재속도에 동기화합니다.", "../assets/offroad/icon_shell.png", Params().read_db_bool("CruiseOverMaxSpeed")) {
    QObject::connect(this, &CruiseOverMaxSpeedToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("CruiseOverMaxSpeed", &value, 1);
    });
  }
};

class MapDecelOnlyToggle : public ToggleControl {
  Q_OBJECT

public:
  MapDecelOnlyToggle() : ToggleControl("가변크루즈 사용시 맵 감속만 사용", "가변크루즈 사용중 맵 감속기능만 사용합니다. 오파모드에서는 동작하지 않습니다.", "../assets/offroad/icon_shell.png", Params().read_db_bool("OpkrMapDecelOnly")) {
    QObject::connect(this, &MapDecelOnlyToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("OpkrMapDecelOnly", &value, 1);
    });
  }
};

class DebugUiOneToggle : public ToggleControl {
  Q_OBJECT

public:
  DebugUiOneToggle() : ToggleControl("DEBUG UI 1", "", "../assets/offroad/icon_shell.png", Params().read_db_bool("DebugUi1")) {
    QObject::connect(this, &DebugUiOneToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("DebugUi1", &value, 1);
    });
  }
};

class DebugUiTwoToggle : public ToggleControl {
  Q_OBJECT

public:
  DebugUiTwoToggle() : ToggleControl("DEBUG UI 2", "", "../assets/offroad/icon_shell.png", Params().read_db_bool("DebugUi2")) {
    QObject::connect(this, &DebugUiTwoToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("DebugUi2", &value, 1);
    });
  }
};

class PrebuiltToggle : public ToggleControl {
  Q_OBJECT

public:
  PrebuiltToggle() : ToggleControl("Prebuilt 파일 생성", "Prebuilt 파일을 생성하며 부팅속도를 단축시킵니다. UI수정을 한 경우 기능을 끄십시오.", "../assets/offroad/icon_shell.png", Params().read_db_bool("PutPrebuiltOn")) {
    QObject::connect(this, &PrebuiltToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("PutPrebuiltOn", &value, 1);
    });
  }
};

class FPToggle : public ToggleControl {
  Q_OBJECT

public:
  FPToggle() : ToggleControl("FingerPrint 이슈 차량 전용", "핑거프린트 이슈차량 전용입니다. 차량인식 문제시 이옵션을 켜고 values.py파일에 핑거프린트를 별도로 넣으십시오.", "../assets/offroad/icon_shell.png", Params().read_db_bool("FingerprintIssuedFix")) {
    QObject::connect(this, &FPToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("FingerprintIssuedFix", &value, 1);
    });
  }
};

class LDWSToggle : public ToggleControl {
  Q_OBJECT

public:
  LDWSToggle() : ToggleControl("LDWS 차량 설정", "", "../assets/offroad/icon_shell.png", Params().read_db_bool("LdwsCarFix")) {
    QObject::connect(this, &LDWSToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("LdwsCarFix", &value, 1);
    });
  }
};

class FPTwoToggle : public ToggleControl {
  Q_OBJECT

public:
  FPTwoToggle() : ToggleControl("FingerPrint 2.0 설정", "핑거프린트2.0을 활성화 합니다. ECU인식으로 차량을 활성화 합니다.", "../assets/offroad/icon_shell.png", Params().read_db_bool("FingerprintTwoSet")) {
    QObject::connect(this, &FPTwoToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("FingerprintTwoSet", &value, 1);
    });
  }
};

class GearDToggle : public ToggleControl {
  Q_OBJECT

public:
  GearDToggle() : ToggleControl("드라이브기어 강제인식", "기어인식문제로 인게이지가 되지 않을 때 사용합니다. 근본적으로 CABANA데이터를 분석해야 하지만, 임시적으로 해결합니다.", "../assets/offroad/icon_shell.png", Params().read_db_bool("JustDoGearD")) {
    QObject::connect(this, &GearDToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("JustDoGearD", &value, 1);
    });
  }
};

class ComIssueToggle : public ToggleControl {
  Q_OBJECT

public:
  ComIssueToggle() : ToggleControl("프로세스간 통신오류 알람 끄기", "화이트판다 사용시 프로세스간 통신오류 알람을 끄기 위해 이옵션을 켜십시오.", "../assets/offroad/icon_shell.png", Params().read_db_bool("ComIssueGone")) {
    QObject::connect(this, &ComIssueToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().write_db_value("ComIssueGone", &value, 1);
    });
  }
};

// SSH key management widget
class SshControl : public AbstractControl {
  Q_OBJECT

public:
  SshControl();

private:
  QPushButton btn;
  QString username;
  QLabel username_label;

  // networking
  QTimer* networkTimer;
  QNetworkReply* reply;
  QNetworkAccessManager* manager;

  void refresh();
  void getUserKeys(QString username);

signals:
  void failedResponse(QString errorString);

private slots:
  void timeout();
  void parseResponse();
};


// 차량 강제등록
class CarForceSet : public AbstractControl {
  Q_OBJECT

public:
  CarForceSet();

private:
  QPushButton btnc;
  QString carname;
  //QLabel carname_label;

  void refreshc();
};


// UI 설정
class AutoShutdown : public AbstractControl {
  Q_OBJECT

public:
  AutoShutdown();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class AutoScreenOff : public AbstractControl {
  Q_OBJECT

public:
  AutoScreenOff();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class VolumeControl : public AbstractControl {
  Q_OBJECT

public:
  VolumeControl();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class BrightnessControl : public AbstractControl {
  Q_OBJECT

public:
  BrightnessControl();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class ChargingMin : public AbstractControl {
  Q_OBJECT

public:
  ChargingMin();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};
class ChargingMax : public AbstractControl {
  Q_OBJECT

public:
  ChargingMax();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};


// 주행 설정
class CruisemodeSelInit : public AbstractControl {
  Q_OBJECT

public:
  CruisemodeSelInit();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class VariableCruiseProfile : public AbstractControl {
  Q_OBJECT

public:
  VariableCruiseProfile();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class LaneChangeSpeed : public AbstractControl {
  Q_OBJECT

public:
  LaneChangeSpeed();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class LaneChangeDelay : public AbstractControl {
  Q_OBJECT

public:
  LaneChangeDelay();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class LeftCurvOffset : public AbstractControl {
  Q_OBJECT

public:
  LeftCurvOffset();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};
class RightCurvOffset : public AbstractControl {
  Q_OBJECT

public:
  RightCurvOffset();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class MaxAngleLimit : public AbstractControl {
  Q_OBJECT

public:
  MaxAngleLimit();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class SpeedLimitOffset : public AbstractControl {
  Q_OBJECT

public:
  SpeedLimitOffset();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

// 튜닝 설정
class CameraOffset : public AbstractControl {
  Q_OBJECT

public:
  CameraOffset();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class SRBaseControl : public AbstractControl {
  Q_OBJECT

public:
  SRBaseControl();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};
class SRMaxControl : public AbstractControl {
  Q_OBJECT

public:
  SRMaxControl();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class SteerActuatorDelay : public AbstractControl {
  Q_OBJECT

public:
  SteerActuatorDelay();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class SteerRateCost : public AbstractControl {
  Q_OBJECT

public:
  SteerRateCost();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class SteerLimitTimer : public AbstractControl {
  Q_OBJECT

public:
  SteerLimitTimer();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class TireStiffnessFactor : public AbstractControl {
  Q_OBJECT

public:
  TireStiffnessFactor();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class SteerMaxBase : public AbstractControl {
  Q_OBJECT

public:
  SteerMaxBase();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class SteerMaxMax : public AbstractControl {
  Q_OBJECT

public:
  SteerMaxMax();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class SteerMaxv : public AbstractControl {
  Q_OBJECT

public:
  SteerMaxv();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class SteerDeltaUpBase : public AbstractControl {
  Q_OBJECT

public:
  SteerDeltaUpBase();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class SteerDeltaUpMax : public AbstractControl {
  Q_OBJECT

public:
  SteerDeltaUpMax();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class SteerDeltaDownBase : public AbstractControl {
  Q_OBJECT

public:
  SteerDeltaDownBase();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class SteerDeltaDownMax : public AbstractControl {
  Q_OBJECT

public:
  SteerDeltaDownMax();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};


// 제어 설정
class LateralControl : public AbstractControl {
  Q_OBJECT

public:
  LateralControl();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class PidKp : public AbstractControl {
  Q_OBJECT

public:
  PidKp();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class PidKi : public AbstractControl {
  Q_OBJECT

public:
  PidKi();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class PidKd : public AbstractControl {
  Q_OBJECT

public:
  PidKd();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class PidKf : public AbstractControl {
  Q_OBJECT

public:
  PidKf();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class IgnoreZone : public AbstractControl {
  Q_OBJECT

public:
  IgnoreZone();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class OuterLoopGain : public AbstractControl {
  Q_OBJECT

public:
  OuterLoopGain();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class InnerLoopGain : public AbstractControl {
  Q_OBJECT

public:
  InnerLoopGain();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class TimeConstant : public AbstractControl {
  Q_OBJECT

public:
  TimeConstant();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class ActuatorEffectiveness : public AbstractControl {
  Q_OBJECT

public:
  ActuatorEffectiveness();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class Scale : public AbstractControl {
  Q_OBJECT

public:
  Scale();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class LqrKi : public AbstractControl {
  Q_OBJECT

public:
  LqrKi();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class DcGain : public AbstractControl {
  Q_OBJECT

public:
  DcGain();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class SteerAngleCorrection : public AbstractControl {
  Q_OBJECT

public:
  SteerAngleCorrection();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class MaxSteer : public AbstractControl {
  Q_OBJECT

public:
  MaxSteer();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class MaxRTDelta : public AbstractControl {
  Q_OBJECT

public:
  MaxRTDelta();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class MaxRateUp : public AbstractControl {
  Q_OBJECT

public:
  MaxRateUp();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class MaxRateDown : public AbstractControl {
  Q_OBJECT

public:
  MaxRateDown();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

class SteerThreshold : public AbstractControl {
  Q_OBJECT

public:
  SteerThreshold();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};