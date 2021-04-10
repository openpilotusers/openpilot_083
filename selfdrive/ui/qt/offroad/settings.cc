#include <string>
#include <iostream>
#include <sstream>
#include <cassert>
#include <QProcess>

#ifndef QCOM
#include "networking.hpp"
#endif
#include "settings.hpp"
#include "widgets/input.hpp"
#include "widgets/toggle.hpp"
#include "widgets/offroad_alerts.hpp"
#include "widgets/controls.hpp"
#include "widgets/ssh_keys.hpp"
#include "common/params.h"
#include "common/util.h"
#include "selfdrive/hardware/hw.h"


QWidget * toggles_panel() {
  QVBoxLayout *toggles_list = new QVBoxLayout();

  toggles_list->addWidget(new ParamControl("OpenpilotEnabledToggle",
                                            "오픈파일럿 사용",
                                            "어댑티브 크루즈 컨트롤 및 차선 유지 지원을 위해 오픈파일럿 시스템을 사용하십시오. 이 기능을 사용하려면 항상 주의를 기울여야 합니다. 이 설정을 변경하는 것은 자동차의 전원이 꺼졌을 때 적용됩니다.",
                                            "../assets/offroad/icon_openpilot.png"
                                              ));
  toggles_list->addWidget(horizontal_line());
  toggles_list->addWidget(new ParamControl("IsLdwEnabled",
                                            "차선이탈 경보 사용",
                                            "50km/h이상의 속도로 주행하는 동안 방향 지시등이 활성화되지 않은 상태에서 차량이 감지된 차선 위를 넘어갈 경우 원래 차선으로 다시 방향을 전환하도록 경고를 보냅니다.",
                                            "../assets/offroad/icon_warning.png"
                                              ));
  toggles_list->addWidget(horizontal_line());
  toggles_list->addWidget(new ParamControl("IsRHD",
                                            "우핸들 운전방식 사용",
                                            "오픈파일럿이 좌측 교통 규칙을 준수하도록 허용하고 우측 운전석에서 운전자 모니터링을 수행하십시오.",
                                            "../assets/offroad/icon_openpilot_mirrored.png"
                                            ));
  toggles_list->addWidget(horizontal_line());
  toggles_list->addWidget(new ParamControl("IsMetric",
                                            "미터법 사용",
                                            "mi/h 대신 km/h 단위로 속도를 표시합니다.",
                                            "../assets/offroad/icon_metric.png"
                                            ));
  toggles_list->addWidget(horizontal_line());
  toggles_list->addWidget(new ParamControl("CommunityFeaturesToggle",
                                            "커뮤니티 기능 사용",
                                            "comma.ai에서 유지 또는 지원하지 않고 표준 안전 모델에 부합하는 것으로 확인되지 않은 오픈 소스 커뮤니티의 기능을 사용하십시오. 이러한 기능에는 커뮤니티 지원 자동차와 커뮤니티 지원 하드웨어가 포함됩니다. 이러한 기능을 사용할 때는 각별히 주의해야 합니다.",
                                            "../assets/offroad/icon_shell.png"
                                            ));
  toggles_list->addWidget(horizontal_line());
  ParamControl *record_toggle = new ParamControl("RecordFront",
                                            "운전자 영상 녹화 및 업로드",
                                            "운전자 모니터링 카메라에서 데이터를 업로드하고 운전자 모니터링 알고리즘을 개선하십시오.",
                                            "../assets/offroad/icon_network.png");
  toggles_list->addWidget(record_toggle);
  toggles_list->addWidget(horizontal_line());
  toggles_list->addWidget(new ParamControl("EndToEndToggle",
                                           "\U0001f96c 차선 비활성화 모드 (알파) \U0001f96c",
                                           "이 모드에서 오픈파일럿은 차선을 따라 주행하지 않고 사람이 운전하는 것 처럼 주행합니다.",
                                           "../assets/offroad/icon_road.png"));
  toggles_list->addWidget(horizontal_line());
  toggles_list->addWidget(new ParamControl("OpkrEnableDriverMonitoring",
                                           "운전자 모니터링 사용",
                                           "운전자 감시 모니터링을 사용합니다.",
                                           "../assets/offroad/icon_shell.png"));
  toggles_list->addWidget(horizontal_line());
  toggles_list->addWidget(new ParamControl("OpkrEnableLogger",
                                           "로그기록 및 업로드 사용",
                                           "주행로그를 기록 후 콤마서버로 전송합니다.",
                                           "../assets/offroad/icon_shell.png"));

  bool record_lock = Params().read_db_bool("RecordFrontLock");
  record_toggle->setEnabled(!record_lock);

  QWidget *widget = new QWidget;
  widget->setLayout(toggles_list);
  return widget;
}

DevicePanel::DevicePanel(QWidget* parent) : QWidget(parent) {
  QVBoxLayout *device_layout = new QVBoxLayout;

  Params params = Params();

  QString dongle = QString::fromStdString(params.get("DongleId", false));
  device_layout->addWidget(new LabelControl("Dongle ID", dongle));
  device_layout->addWidget(horizontal_line());

  QString serial = QString::fromStdString(params.get("HardwareSerial", false));
  device_layout->addWidget(new LabelControl("Serial", serial));

  // offroad-only buttons
  QList<ButtonControl*> offroad_btns;

  offroad_btns.append(new ButtonControl("운전자 영상", "미리보기",
                                   "운전자 모니터링 카메라를 미리 보고 장치 장착 위치를 최적화하여 최상의 운전자 모니터링 환경을 제공하십시오. (차량이 꺼져 있어야 합니다.)",
                                   [=]() { Params().write_db_value("IsDriverViewEnabled", "1", 1); }));

//  offroad_btns.append(new ButtonControl("캘리브레이션 초기화", "리셋",
//                                   "오픈파일럿을 사용하려면 장치를 왼쪽 또는 오른쪽으로 4°, 위 또는 아래로 5° 이내에 장착해야 합니다. 오픈파일럿이 지속적으로 보정되고 있으므로 재설정할 필요가 거의 없습니다.", [=]() {
//    if (ConfirmationDialog::confirm("캘리브레이션을 초기화 하시겠습니까?")) {
//      Params().delete_db_value("CalibrationParams");
//    }
//  }));

  offroad_btns.append(new ButtonControl("트레이닝가이드 보기", "다시보기",
                                        "오픈파일럿에 대한 규칙, 기능, 제한내용 등을 확인하세요.", [=]() {
    if (ConfirmationDialog::confirm("트레이닝 가이드를 다시 확인하시겠습니까?")) {
      Params().delete_db_value("CompletedTrainingVersion");
      emit reviewTrainingGuide();
    }
  }));

  QString brand = params.read_db_bool("Passive") ? "대시캠" : "오픈파일럿";
  offroad_btns.append(new ButtonControl(brand + " 제거", "제거", "", [=]() {
    if (ConfirmationDialog::confirm("제거하시겠습니까?")) {
      Params().write_db_value("DoUninstall", "1");
    }
  }));

  for(auto &btn : offroad_btns){
    device_layout->addWidget(horizontal_line());
    QObject::connect(parent, SIGNAL(offroadTransition(bool)), btn, SLOT(setEnabled(bool)));
    device_layout->addWidget(btn);
  }

  device_layout->addWidget(horizontal_line());

  // cal reset and param init buttons
  QHBoxLayout *cal_param_init_layout = new QHBoxLayout();
  cal_param_init_layout->setSpacing(50);

  QPushButton *calinit_btn = new QPushButton("캘리브레이션 리셋");
  cal_param_init_layout->addWidget(calinit_btn);
  QObject::connect(calinit_btn, &QPushButton::released, [=]() {
    if (ConfirmationDialog::confirm("캘리브레이션을 초기화할까요? 자동 재부팅됩니다.")) {
      Params().delete_db_value("CalibrationParams");
      QProcess::execute("reboot");
    }
  });

  QPushButton *paraminit_btn = new QPushButton("파라미터 초기화");
  cal_param_init_layout->addWidget(paraminit_btn);
  QObject::connect(paraminit_btn, &QPushButton::released, [=]() {
    if (ConfirmationDialog::confirm("파라미터를 초기상태로 되돌립니다. 진행하시겠습니까?")) {
      QProcess::execute("/data/openpilot/init_param.sh");
    }
  });

  // preset1 buttons
  QHBoxLayout *presetone_layout = new QHBoxLayout();
  presetone_layout->setSpacing(50);

  QPushButton *presetoneload_btn = new QPushButton("프리셋1 불러오기");
  presetone_layout->addWidget(presetoneload_btn);
  QObject::connect(presetoneload_btn, &QPushButton::released, [=]() {
    if (ConfirmationDialog::confirm("프리셋1을 불러올까요?")) {
      QProcess::execute("/data/openpilot/load_preset1.sh");
    }
  });

  QPushButton *presetonesave_btn = new QPushButton("프리셋1 저장하기");
  presetone_layout->addWidget(presetonesave_btn);
  QObject::connect(presetonesave_btn, &QPushButton::released, [=]() {
    if (ConfirmationDialog::confirm("프리셋1을 저장할까요?")) {
      QProcess::execute("/data/openpilot/save_preset1.sh");
    }
  });

  // preset2 buttons
  QHBoxLayout *presettwo_layout = new QHBoxLayout();
  presettwo_layout->setSpacing(50);

  QPushButton *presettwoload_btn = new QPushButton("프리셋2 불러오기");
  presettwo_layout->addWidget(presettwoload_btn);
  QObject::connect(presettwoload_btn, &QPushButton::released, [=]() {
    if (ConfirmationDialog::confirm("프리셋2을 불러올까요?")) {
      QProcess::execute("/data/openpilot/load_preset2.sh");
    }
  });

  QPushButton *presettwosave_btn = new QPushButton("프리셋2 저장하기");
  presettwo_layout->addWidget(presettwosave_btn);
  QObject::connect(presettwosave_btn, &QPushButton::released, [=]() {
    if (ConfirmationDialog::confirm("프리셋2을 저장할까요?")) {
      QProcess::execute("/data/openpilot/save_preset2.sh");
    }
  });

  // power buttons
  QHBoxLayout *power_layout = new QHBoxLayout();
  power_layout->setSpacing(50);

  QPushButton *reboot_btn = new QPushButton("재시작");
  power_layout->addWidget(reboot_btn);
  QObject::connect(reboot_btn, &QPushButton::released, [=]() {
    if (ConfirmationDialog::confirm("재시작하시겠습니까?")) {
      Hardware::reboot();
    }
  });

  QPushButton *poweroff_btn = new QPushButton("전원끄기");
  poweroff_btn->setStyleSheet("background-color: #E22C2C;");
  power_layout->addWidget(poweroff_btn);
  QObject::connect(poweroff_btn, &QPushButton::released, [=]() {
    if (ConfirmationDialog::confirm("전원을 끄시겠습니까?")) {
      Hardware::poweroff();
    }
  });

  device_layout->addLayout(cal_param_init_layout);

  device_layout->addWidget(horizontal_line());

  device_layout->addLayout(presetone_layout);
  device_layout->addLayout(presettwo_layout);

  device_layout->addWidget(horizontal_line());

  device_layout->addLayout(power_layout);

  setLayout(device_layout);
  setStyleSheet(R"(
    QPushButton {
      padding: 20;
      height: 120px;
      border-radius: 15px;
      background-color: #393939;
    }
  )");
}

DeveloperPanel::DeveloperPanel(QWidget* parent) : QFrame(parent) {
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  setLayout(main_layout);
  setStyleSheet(R"(QLabel {font-size: 50px;})");
}

void DeveloperPanel::showEvent(QShowEvent *event) {
  Params params = Params();
  std::string brand = params.read_db_bool("Passive") ? "대시캠" : "오픈파일럿";
  QList<QPair<QString, std::string>> dev_params = {
    {"Version", brand + " v" + params.get("Version", false).substr(0, 14)},
    {"Git Branch", params.get("GitBranch", false)},
    {"Git Commit", params.get("GitCommit", false).substr(0, 10)},
    {"Panda Firmware", params.get("PandaFirmwareHex", false)},
    {"OS Version", Hardware::get_os_version()},
  };

  for (int i = 0; i < dev_params.size(); i++) {
    const auto &[name, value] = dev_params[i];
    QString val = QString::fromStdString(value).trimmed();
    if (labels.size() > i) {
      labels[i]->setText(val);
    } else {
      labels.push_back(new LabelControl(name, val));
      layout()->addWidget(labels[i]);
      if (i < (dev_params.size() - 1)) {
        layout()->addWidget(horizontal_line());
      }
    }
  }
}

QWidget * network_panel(QWidget * parent) {
#ifdef QCOM
  QVBoxLayout *layout = new QVBoxLayout;
  layout->setSpacing(30);

  layout->addWidget(new OpenpilotView());
  layout->addWidget(horizontal_line());
  // wifi + tethering buttons
  layout->addWidget(new ButtonControl("WiFi 설정", "열기", "",
                                      [=]() { HardwareEon::launch_wifi(); }));
  layout->addWidget(horizontal_line());

  layout->addWidget(new ButtonControl("테더링 설정", "열기", "",
                                      [=]() { HardwareEon::launch_tethering(); }));
  layout->addWidget(horizontal_line());

  // SSH key management
  layout->addWidget(new SshToggle());
  layout->addWidget(horizontal_line());
  layout->addWidget(new SshControl());
  layout->addWidget(horizontal_line());
  layout->addWidget(new SshLegacyToggle());
  layout->addWidget(horizontal_line());

  const char* gitpull = "/data/openpilot/gitpull.sh ''";
  layout->addWidget(new ButtonControl("Git Pull", "실행", "리모트 Git에서 변경사항이 있으면 로컬에 반영 후 자동 재부팅 됩니다. 변경사항이 없으면 재부팅하지 않습니다. 로컬 파일이 변경된경우 리모트Git 내역을 반영 못할수도 있습니다. 참고바랍니다.",
                                      [=]() { 
                                        if (ConfirmationDialog::confirm("Git에서 변경사항 적용 후 자동 재부팅 됩니다. 없으면 재부팅하지 않습니다. 진행하시겠습니까?")){
                                          std::system(gitpull);
                                        }
                                      }));

  layout->addWidget(horizontal_line());

  const char* git_reset = "/data/openpilot/git_reset.sh ''";
  layout->addWidget(new ButtonControl("Git Reset", "실행", "로컬변경사항을 강제 초기화 후 리모트 최신 커밋내역을 적용합니다. 로컬 변경사항이 사라지니 주의 바랍니다.",
                                      [=]() { 
                                        if (ConfirmationDialog::confirm("로컬변경사항을 강제 초기화 후 리모트Git의 최신 커밋내역을 적용합니다. 진행하시겠습니까?")){
                                          std::system(git_reset);
                                        }
                                      }));


  layout->addWidget(horizontal_line());

  const char* gitpull_cancel = "/data/openpilot/gitpull_cancel.sh ''";
  layout->addWidget(new ButtonControl("Git Pull 취소", "실행", "Git Pull을 취소하고 이전상태로 되돌립니다. 커밋내역이 여러개인경우 최신커밋 바로 이전상태로 되돌립니다.",
                                      [=]() { 
                                        if (ConfirmationDialog::confirm("GitPull 이전 상태로 되돌립니다. 진행하시겠습니까?")){
                                          std::system(gitpull_cancel);
                                        }
                                      }));

  layout->addWidget(horizontal_line());

//  const char* param_init = "/data/openpilot/init_param.sh ''";
//  layout->addWidget(new ButtonControl("파라미터 초기화", "실행", "파라미터를 처음 설치한 상태로 되돌립니다.",
//                                      [=]() { 
//                                        if (ConfirmationDialog::confirm("파라미터를 처음 설치한 상태로 되돌립니다. 진행하시겠습니까?")){
//                                          std::system(param_init);
//                                        }
//                                      }));
//
//  layout->addWidget(horizontal_line());

  const char* panda_flashing = "/data/openpilot/panda_flashing.sh ''";
  layout->addWidget(new ButtonControl("판다 플래싱", "실행", "판다플래싱이 진행되면 판다의 녹색LED가 빠르게 깜빡이며 완료되면 자동 재부팅 됩니다. 절대로 장치의 전원을 끄거나 임의로 분리하지 마시기 바랍니다.",
                                      [=]() {
                                        if (ConfirmationDialog::confirm("판다플래싱을 진행하시겠습니까?")) {
                                          std::system(panda_flashing);
                                        }
                                      }));

  layout->addStretch(1);

  QWidget *w = new QWidget;
  w->setLayout(layout);
#else
  Networking *w = new Networking(parent);
#endif
  return w;
}

QWidget * user_panel(QWidget * parent) {
  QVBoxLayout *layout = new QVBoxLayout;

  // OPKR
  layout->addWidget(new LabelControl("UI설정", ""));
  layout->addWidget(new AutoShutdown());
  layout->addWidget(new AutoScreenDimmingToggle());
  layout->addWidget(new VolumeControl());
  layout->addWidget(new BrightnessControl());
  layout->addWidget(new GetoffAlertToggle());
  layout->addWidget(new BatteryChargingControlToggle());
  layout->addWidget(new ChargingMin());
  layout->addWidget(new ChargingMax());
  layout->addWidget(new DrivingRecordToggle());
  layout->addWidget(new HotspotOnBootToggle());

  layout->addWidget(horizontal_line());
  layout->addWidget(new LabelControl("주행설정", ""));
  layout->addWidget(new AutoResumeToggle());
  layout->addWidget(new VariableCruiseToggle());
  layout->addWidget(new VariableCruiseProfile());
  layout->addWidget(new CruisemodeSelInit());
  layout->addWidget(new LaneChangeSpeed());
  layout->addWidget(new LaneChangeDelay());
  layout->addWidget(new LeftCurvOffset());
  layout->addWidget(new RightCurvOffset());
  layout->addWidget(new BlindSpotDetectToggle());
  layout->addWidget(new MaxAngleLimit());
  layout->addWidget(new SteerAngleCorrection());
  layout->addWidget(new TurnSteeringDisableToggle());
  layout->addWidget(new CruiseOverMaxSpeedToggle());
  layout->addWidget(new SpeedLimitOffset());
  layout->addWidget(new MapDecelOnlyToggle());

  layout->addWidget(horizontal_line());
  layout->addWidget(new LabelControl("개발자", ""));
  layout->addWidget(new DebugUiOneToggle());
  layout->addWidget(new DebugUiTwoToggle());
  layout->addWidget(new PrebuiltToggle());
  layout->addWidget(new FPToggle());
  layout->addWidget(new FPTwoToggle());
  layout->addWidget(new LDWSToggle());
  layout->addWidget(new GearDToggle());
  layout->addWidget(new ComIssueToggle());
  const char* cal_ok = "cp -f /data/openpilot/selfdrive/assets/addon/param/CalibrationParams /data/params/d/";
  layout->addWidget(new ButtonControl("캘리브레이션 강제 활성화", "실행", "실주행으로 캘리브레이션을 설정하지 않고 이온을 초기화 한경우 인게이지 확인용도로 캘리브레이션을 강제 설정합니다.",
                                      [=]() { 
                                        if (ConfirmationDialog::confirm("캘리브레이션을 강제로 설정합니다. 인게이지 확인용이니 실 주행시에는 초기화 하시기 바랍니다.")){
                                          std::system(cal_ok);
                                        }
                                      }));
  layout->addWidget(horizontal_line());
  layout->addWidget(new CarForceSet());
  QString car_model = QString::fromStdString(Params().get("CarModel", false));
  layout->addWidget(new LabelControl("현재차량모델", ""));
  layout->addWidget(new LabelControl(car_model, ""));
  layout->addWidget(horizontal_line());
  layout->addWidget(new LabelControl("판다 값", "주의要"));
  layout->addWidget(new MaxSteer());
  layout->addWidget(new MaxRTDelta());
  layout->addWidget(new MaxRateUp());
  layout->addWidget(new MaxRateDown());
  const char* p_edit_go = "/data/openpilot/p_edit.sh ''";
  layout->addWidget(new ButtonControl("판다 값 일괄 변경 적용", "실행", "변경된 판다값을 적용합니다. 콤마가 좋아하지 않으니 사용에 주의를 요합니다.",
                                      [=]() { 
                                        if (ConfirmationDialog::confirm("변경된 판다값을 적용합니다. 콤마가 안좋아합니다. 적용후 자동 재부팅 됩니다.")){
                                          std::system(p_edit_go);
                                        }
                                      }));
  //layout->addWidget(horizontal_line());

  layout->addStretch(1);

  QWidget *w = new QWidget;
  w->setLayout(layout);

  return w;
}

QWidget * tuning_panel(QWidget * parent) {
  QVBoxLayout *layout = new QVBoxLayout;

  // OPKR
  layout->addWidget(new LabelControl("튜닝메뉴", ""));
  layout->addWidget(new CameraOffset());
  layout->addWidget(new LiveSteerRatioToggle());
  layout->addWidget(new SRBaseControl());
  layout->addWidget(new SRMaxControl());
  layout->addWidget(new SteerActuatorDelay());
  layout->addWidget(new SteerRateCost());
  layout->addWidget(new SteerLimitTimer());
  layout->addWidget(new TireStiffnessFactor());
  layout->addWidget(new SteerMaxBase());
  layout->addWidget(new SteerMaxMax());
  layout->addWidget(new SteerMaxv());
  layout->addWidget(new VariableSteerMaxToggle());
  layout->addWidget(new SteerDeltaUpBase());
  layout->addWidget(new SteerDeltaUpMax());
  layout->addWidget(new SteerDeltaDownBase());
  layout->addWidget(new SteerDeltaDownMax());
  layout->addWidget(new VariableSteerDeltaToggle());
  layout->addWidget(new SteerThreshold());

  layout->addWidget(horizontal_line());

  layout->addWidget(new LabelControl("제어메뉴", ""));
  layout->addWidget(new LateralControl());
  layout->addWidget(new LiveTuneToggle());
  QString lat_control = QString::fromStdString(Params().get("LateralControlMethod", false));
  if (lat_control == "0") {
    layout->addWidget(new PidKp());
    layout->addWidget(new PidKi());
    layout->addWidget(new PidKd());
    layout->addWidget(new PidKf());
    layout->addWidget(new IgnoreZone());
    layout->addWidget(new ShaneFeedForward());
  } else if (lat_control == "1") {
    layout->addWidget(new InnerLoopGain());
    layout->addWidget(new OuterLoopGain());
    layout->addWidget(new TimeConstant());
    layout->addWidget(new ActuatorEffectiveness());
  } else if (lat_control == "2") {
    layout->addWidget(new Scale());
    layout->addWidget(new LqrKi());
    layout->addWidget(new DcGain());
  }

  layout->addStretch(1);

  QWidget *w = new QWidget;
  w->setLayout(layout);

  return w;
}

SettingsWindow::SettingsWindow(QWidget *parent) : QFrame(parent) {
  // setup two main layouts
  QVBoxLayout *sidebar_layout = new QVBoxLayout();
  sidebar_layout->setMargin(0);
  panel_widget = new QStackedWidget();
  panel_widget->setStyleSheet(R"(
    border-radius: 30px;
    background-color: #292929;
  )");

  // close button
  QPushButton *close_btn = new QPushButton("닫기");
  close_btn->setStyleSheet(R"(
    font-size: 60px;
    font-weight: bold;
    border 1px grey solid;
    border-radius: 100px;
    background-color: #292929;
  )");
  close_btn->setFixedSize(200, 200);
  sidebar_layout->addSpacing(45);
  sidebar_layout->addWidget(close_btn, 0, Qt::AlignCenter);
  QObject::connect(close_btn, SIGNAL(released()), this, SIGNAL(closeSettings()));

  // setup panels
  DevicePanel *device = new DevicePanel(this);
  QObject::connect(device, SIGNAL(reviewTrainingGuide()), this, SIGNAL(reviewTrainingGuide()));

  QPair<QString, QWidget *> panels[] = {
    {"장치", new DevicePanel(this)},
    {"네트워크", network_panel(this)},
    {"토글메뉴", toggles_panel()},
    {"정보", new DeveloperPanel()},
    {"사용자설정", user_panel(this)},
    {"튜닝", tuning_panel(this)},
  };

  sidebar_layout->addSpacing(45);
  nav_btns = new QButtonGroup();
  for (auto &[name, panel] : panels) {
    QPushButton *btn = new QPushButton(name);
    btn->setCheckable(true);
    btn->setStyleSheet(R"(
      QPushButton {
        color: grey;
        border: none;
        background: none;
        font-size: 65px;
        font-weight: 500;
        padding-top: 18px;
        padding-bottom: 18px;
      }
      QPushButton:checked {
        color: white;
      }
    )");

    nav_btns->addButton(btn);
    sidebar_layout->addWidget(btn, 0, Qt::AlignRight);

    panel->setContentsMargins(50, 25, 50, 25);
    QScrollArea *panel_frame = new QScrollArea;
    panel_frame->setWidget(panel);
    panel_frame->setWidgetResizable(true);
    panel_frame->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    panel_frame->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    panel_frame->setStyleSheet("background-color:transparent;");

    QScroller *scroller = QScroller::scroller(panel_frame->viewport());
    auto sp = scroller->scrollerProperties();

    sp.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy, QVariant::fromValue<QScrollerProperties::OvershootPolicy>(QScrollerProperties::OvershootAlwaysOff));

    scroller->grabGesture(panel_frame->viewport(), QScroller::LeftMouseButtonGesture);
    scroller->setScrollerProperties(sp);

    panel_widget->addWidget(panel_frame);

    QObject::connect(btn, &QPushButton::released, [=, w = panel_frame]() {
      panel_widget->setCurrentWidget(w);
    });
  }
  qobject_cast<QPushButton *>(nav_btns->buttons()[0])->setChecked(true);
  sidebar_layout->setContentsMargins(50, 50, 100, 50);

  // main settings layout, sidebar + main panel
  QHBoxLayout *settings_layout = new QHBoxLayout();

  sidebar_widget = new QWidget;
  sidebar_widget->setLayout(sidebar_layout);
  sidebar_widget->setFixedWidth(500);
  settings_layout->addWidget(sidebar_widget);
  settings_layout->addWidget(panel_widget);

  setLayout(settings_layout);
  setStyleSheet(R"(
    * {
      color: white;
      font-size: 50px;
    }
    SettingsWindow {
      background-color: black;
    }
  )");
}
