
#include <string>
#include <iostream>
#include <sstream>
#include <cassert>



#include "widgets/input.hpp"
#include "widgets/toggle.hpp"
#include "widgets/offroad_alerts.hpp"
#include "widgets/controls.hpp"

#include "common/params.h"
#include "common/util.h"


#include "userPanel.hpp"


UserPanel::UserPanel(QWidget* parent) : QFrame(parent) 
{
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  main_layout->setMargin(100);
  setLayout(main_layout);

  layout()->addWidget(new ButtonControl("* PROGRAM DOWNLOAD", "<git pull>",
                                        "git 으로 부터 프로그램을 Download합니다.", [=]() 
                                        {
                                            if (ConfirmationDialog::confirm("Are you sure you want to git pull?")) 
                                            {
                                              std::system("git pull");
                                            }
                                        }
                                       )
                      ); 

  layout()->addWidget(horizontal_line());

  layout()->addWidget(new ParamControl("IsOpenpilotViewEnabled",
                                       "주행화면 미리보기",
                                       "오픈파일럿 주행화면을 미리보기 합니다.",
                                       "../assets/offroad/icon_eon.png"
                                       ));

  layout()->addWidget(horizontal_line());

   layout()->addWidget(new CAutoResumeToggle());
   layout()->addWidget(new CLiveSteerRatioToggle());
   layout()->addWidget(new CTurnSteeringDisableToggle());
   layout()->addWidget(new CPrebuiltToggle());
}

void UserPanel::showEvent(QShowEvent *event) 
{
  Params params = Params();


}

/*
QWidget * user_panel(QWidget * parent) 
{
  QVBoxLayout *layout = new QVBoxLayout;

  layout->setMargin(100);
  layout->setSpacing(30);

  // OPKR
  std::vector<std::pair<std::string, std::string>> labels1 = {{"토글메뉴(UI)", ""},};
  for (auto &l : labels1) {layout->addWidget(new LabelControl(QString::fromStdString(l.first),
                             QString::fromStdString(l.second)));
  }
  layout->addWidget(new GetoffAlertToggle());
  layout->addWidget(new BatteryChargingControlToggle());
  layout->addWidget(new DrivingRecordToggle());
  layout->addWidget(new HotspotOnBootToggle());

  layout->addWidget(horizontal_line());
  std::vector<std::pair<std::string, std::string>> labels2 = {{"토글메뉴(주행)", ""},};
  for (auto &l : labels2) {layout->addWidget(new LabelControl(QString::fromStdString(l.first),
                             QString::fromStdString(l.second)));
  }
  layout->addWidget(new AutoResumeToggle());
  layout->addWidget(new VariableCruiseToggle());
  layout->addWidget(new BlindSpotDetectToggle());
  layout->addWidget(new TurnSteeringDisableToggle());
  layout->addWidget(new CruiseOverMaxSpeedToggle());
  layout->addWidget(new MapDecelOnlyToggle());

  layout->addWidget(horizontal_line());
  std::vector<std::pair<std::string, std::string>> labels3 = {{"토글메뉴(개발자)", ""},};
  for (auto &l : labels3) {layout->addWidget(new LabelControl(QString::fromStdString(l.first),
                             QString::fromStdString(l.second)));
  }
  layout->addWidget(new DebugUiOneToggle());
  layout->addWidget(new DebugUiTwoToggle());
  layout->addWidget(new PrebuiltToggle());
  layout->addWidget(new FPToggle());
  layout->addWidget(new FPTwoToggle());
  layout->addWidget(new LDWSToggle());
  //layout->addWidget(horizontal_line());

  layout->addStretch(1);

  QWidget *w = new QWidget;
  w->setLayout(layout);

  return w;
}
*/

