#include <time.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#define CAPTURE_STATE_NONE 0
#define CAPTURE_STATE_CAPTURING 1
#define CAPTURE_STATE_NOT_CAPTURING 2
#define CAPTURE_STATE_PAUSED 3
#define CLICK_TIME 0.2
#define RECORD_INTERVAL 180 // Time in seconds to rotate recordings; Max for screenrecord is 3 minutes
#define RECORD_FILES 40     // Number of files to create before looping over

typedef struct dashcam_element
{
  int pos_x;
  int pos_y;
  int width;
  int height;
} dashcam_element;

dashcam_element lock_button;

extern float  fFontSize;

int captureState = CAPTURE_STATE_NOT_CAPTURING;
int captureNum = 0;
int start_time = 0;
int stop_time = 0;
int elapsed_time = 0; // Time of current recording
int click_elapsed_time = 0;
int click_time = 0;
char filenames[RECORD_FILES][50]; // Track the filenames so they can be deleted when rotating

bool lock_current_video = false; // If true save the current video before rotating
bool locked_files[RECORD_FILES]; // Track which files are locked
int lock_image;                  // Stores reference to the PNG
int files_created = 0;
int  capture_cnt = 0;


void ui_print(UIState *s, int x, int y,  const char* fmt, ... )
{
  //char speed_str[512];  
  char* msg_buf = NULL;
  va_list args;
  va_start(args, fmt);
  vasprintf( &msg_buf, fmt, args);
  va_end(args);

  nvgText(s->vg, x, y, msg_buf, NULL);
}


int get_time()
{
  // Get current time (in seconds)

  int iRet;
  struct timeval tv;
  int seconds = 0;

  iRet = gettimeofday(&tv, NULL);
  if (iRet == 0)
  {
    seconds = (int)tv.tv_sec;
  }
  return seconds;
}

struct tm get_time_struct()
{
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  return tm;
}

void remove_file(char *videos_dir, char *filename)
{
  if (filename[0] == '\0')
  {
    // Don't do anything if no filename is passed
    return;
  }

  int status;
  char fullpath[64];
  snprintf(fullpath, sizeof(fullpath), "%s/%s", videos_dir, filename);
  status = remove(fullpath);
  if (status == 0)
  {
    printf("Removed file: %s\n", fullpath);
  }
  else
  {
    printf("Unable to remove file: %s\n", fullpath);
    perror("Error message:");
  }
}

void save_file(char *videos_dir, char *filename)
{
  if (!strlen(filename))
  {
    return;
  }

  // Rename file to save it from being overwritten
  char cmd[128];
  snprintf(cmd, sizeof(cmd), "mv %s/%s %s/saved_%s", videos_dir, filename, videos_dir, filename);
  printf("save: %s\n", cmd);
  system(cmd);
}

void stop_capture() {
  char videos_dir[50] = "/storage/emulated/0/videos";

  

  if (captureState == CAPTURE_STATE_CAPTURING)
  {
    printf("stop_capture()\n ");
    system("killall -SIGINT screenrecord");
    captureState = CAPTURE_STATE_NOT_CAPTURING;
    elapsed_time = get_time() - start_time;
    if (elapsed_time < 3)
    {
      remove_file(videos_dir, filenames[captureNum]);
    }
    else
    {
      //printf("Stop capturing screen\n");
      captureNum++;

      if (captureNum > RECORD_FILES - 1)
      {
        captureNum = 0;
      }
    }
  }
}

void start_capture()
{
  captureState = CAPTURE_STATE_CAPTURING;
  char cmd[128] = "";
  char videos_dir[50] = "/storage/emulated/0/videos";

  printf("start_capture()\n ");

  //////////////////////////////////
  // NOTE: make sure videos_dir folder exists on the device!
  //////////////////////////////////
  struct stat st = {0};
  if (stat(videos_dir, &st) == -1)
  {
    mkdir(videos_dir, 0700);
  }
  /*if (captureNum == 0 && files_created == 0) {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir ("/storage/emulated/0/videos")) != NULL) {
      while ((ent = readdir (dir)) != NULL) {
        strcpy(filenames[files_created++], ent->d_name);
      }
      captureNum = files_created;
      closedir (dir);
    }
  }*/

  if (strlen(filenames[captureNum]) && files_created >= RECORD_FILES)
  {
    if (locked_files[captureNum] > 0)
    {
      save_file(videos_dir, filenames[captureNum]);
    }
    else
    {
      // remove the old file
      remove_file(videos_dir, filenames[captureNum]);
    }
    locked_files[captureNum] = 0;
  }

  char filename[64];
  struct tm tm = get_time_struct();
  snprintf(filename, sizeof(filename), "%04d%02d%02d-%02d%02d%02d.mp4", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  snprintf(cmd, sizeof(cmd), "screenrecord --size 1280x720 --bit-rate 5000000 %s/%s&", videos_dir, filename);
  //snprintf(cmd,sizeof(cmd),"screenrecord --size 960x540 --bit-rate 5000000 %s/%s&",videos_dir,filename);
  strcpy(filenames[captureNum], filename);

  printf("Capturing to file: %s\n", cmd);
  start_time = get_time();
  system(cmd);

  if (lock_current_video)
  {
    // Lock is still on so mark this file for saving
    locked_files[captureNum] = 1;
  }
  else
  {
    locked_files[captureNum] = 0;
  }

  files_created++;
}


bool screen_button_clicked(int touch_x, int touch_y, int x, int y, int cx, int cy )
{
   int max_x = x + cx;
   int max_y = y + cy;
   int min_x = x - cx;
   int min_y = y - cy;


  if (touch_x >= min_x && touch_x <= max_x)
  {
    if (touch_y >= min_y && touch_y <= max_y)
    {
      return true;
    }
  }
  return false;
}

void draw_date_time(UIState *s)
{
  /*
  if (captureState == CAPTURE_STATE_NOT_CAPTURING)
  {
    // Don't draw if we're not recording
    return;
  }
  */

  // Get local time to display
  char now[50];
  struct tm tm = get_time_struct();
  snprintf(now, sizeof(now), "%04d/%02d/%02d  %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);


  nvgFontSize(s->vg, 40*fFontSize);
  nvgFontFace(s->vg, "sans-semibold");
 // nvgFillColor(s->vg, nvgRGBA(255, 255, 255, 200));
  nvgText(s->vg, 1602, 23, now, NULL);
}


static void rotate_video()
{
  // Overwrite the existing video (if needed)
  elapsed_time = 0;
  stop_capture();
  captureState = CAPTURE_STATE_CAPTURING;
  start_capture();
}


static void screen_draw_button(UIState *s, int touch_x, int touch_y)
{
  // Set button to bottom left of screen
  //  if (s->vision_connected && s->plus_state == 0) {
  nvgTextAlign(s->vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
  //if (s->vision_connected)
  //{
    int btn_w = 150;
    int btn_h = 150;
    int btn_x = 1920 - btn_w;
    int btn_y = 1080 - btn_h;
    nvgBeginPath(s->vg);
    nvgRoundedRect(s->vg, btn_x - 110, btn_y - 45, btn_w, btn_h, 100);
    nvgStrokeColor(s->vg, nvgRGBA(255, 255, 255, 80));
    nvgStrokeWidth(s->vg, 6);
    nvgStroke(s->vg);

    nvgFontSize(s->vg, 60*fFontSize);

    if ( lock_current_video == false )
    {
       nvgFillColor(s->vg, nvgRGBA( 50, 50, 100, 200));
    }
    else if (captureState == CAPTURE_STATE_CAPTURING)
    {
      NVGcolor fillColor = nvgRGBA(255, 0, 0, 150);
      nvgFillColor(s->vg, fillColor);
      nvgFill(s->vg);
      nvgFillColor(s->vg, nvgRGBA(255, 255, 255, 200));
    }
    else
    {
      nvgFillColor(s->vg, nvgRGBA(255, 150, 150, 200));
    }
    nvgText(s->vg, btn_x - 75, btn_y + 50, "REC", NULL);
 // }

  if (captureState == CAPTURE_STATE_CAPTURING)
  {
    draw_date_time(s);
    elapsed_time = get_time() - start_time;
    if (elapsed_time >= RECORD_INTERVAL)
    {
      capture_cnt++;
      if( capture_cnt > 10 )
      {
        stop_capture();
        lock_current_video = false;
      }
      else
      {
        rotate_video(); 
      }
    }    
  }
}

void screen_toggle_record_state()
{
  //if (captureState == CAPTURE_STATE_CAPTURING)
  if( lock_current_video == true )
  {
    stop_capture();
    lock_current_video = false;
  }
  else
  {
    // start_capture();
    lock_current_video = true;
  }
}


static int draw_measure(UIState *s,  const char* bb_value, const char* bb_uom, const char* bb_label,
    int bb_x, int bb_y, int bb_uom_dx,
    NVGcolor bb_valueColor, NVGcolor bb_labelColor, NVGcolor bb_uomColor,
    int bb_valueFontSize, int bb_labelFontSize, int bb_uomFontSize )
    {
  //const UIScene *scene = &s->scene;
  //const UIScene *scene = &s->scene;
  nvgTextAlign(s->vg, NVG_ALIGN_CENTER | NVG_ALIGN_BASELINE);
  int dx = 0;
  if (strlen(bb_uom) > 0) {
    dx = (int)(bb_uomFontSize*2.5/2);
   }
  //print value
  nvgFontFace(s->vg, "sans-semibold");
  nvgFontSize(s->vg, bb_valueFontSize*2*fFontSize);
  nvgFillColor(s->vg, bb_valueColor);
  nvgText(s->vg, bb_x-dx/2, bb_y+ (int)(bb_valueFontSize*2.5)+5, bb_value, NULL);
  //print label
  nvgFontFace(s->vg, "sans-regular");
  nvgFontSize(s->vg, bb_labelFontSize*2*fFontSize);
  nvgFillColor(s->vg, bb_labelColor);
  nvgText(s->vg, bb_x, bb_y + (int)(bb_valueFontSize*2.5)+5 + (int)(bb_labelFontSize*2.5)+5, bb_label, NULL);
  //print uom
  if (strlen(bb_uom) > 0) {
      nvgSave(s->vg);
    int rx =bb_x + bb_uom_dx + bb_valueFontSize -3;
    int ry = bb_y + (int)(bb_valueFontSize*2.5/2)+25;
    nvgTranslate(s->vg,rx,ry);
    nvgRotate(s->vg, -1.5708); //-90deg in radians
    nvgFontFace(s->vg, "sans-regular");
    nvgFontSize(s->vg, (int)(bb_uomFontSize*2*fFontSize));
    nvgFillColor(s->vg, bb_uomColor);
    nvgText(s->vg, 0, 0, bb_uom, NULL);
    nvgRestore(s->vg);
  }
  return (int)((bb_valueFontSize + bb_labelFontSize)*2.5) + 5;
}

static void draw_menu(UIState *s, int bb_x, int bb_y, int bb_w ) 
{
  UIScene &scene = s->scene;
  int bb_rx = bb_x + (int)(bb_w/2);
  int bb_ry = bb_y;
  int bb_h = 5;
  NVGcolor lab_color = nvgRGBA(255, 255, 255, 200);
  NVGcolor uom_color = nvgRGBA(255, 255, 255, 200);
  int value_fontSize=25;
  int label_fontSize=15;
  int uom_fontSize = 15;
  int bb_uom_dx =  (int)(bb_w /2 - uom_fontSize*2.5) ;



  //add visual radar relative distance
  if( true )
  {
    char val_str[50];
    char uom_str[20];
    NVGcolor val_color = nvgRGBA(255, 255, 255, 200);

    snprintf(val_str, sizeof(val_str), "git pull");
    snprintf(uom_str, sizeof(uom_str), "%d", scene.dash_menu_no );
    bb_h +=draw_measure(s,  val_str, uom_str, "git pull",
        bb_rx, bb_ry, bb_uom_dx,
        val_color, lab_color, uom_color,
        value_fontSize, label_fontSize, uom_fontSize );
    bb_ry = bb_y + bb_h;
  }


  //finally draw the frame
  bb_h += 20;
  nvgBeginPath(s->vg);
    nvgRoundedRect(s->vg, bb_x, bb_y, bb_w, bb_h, 20);
    nvgStrokeColor(s->vg, nvgRGBA(255,255,255,80));
    nvgStrokeWidth(s->vg, 6);
    nvgStroke(s->vg);
}

static void screen_menu_button(UIState *s, int touch_x, int touch_y, int touched)
{
  // Set button to bottom left of screen
  UIScene &scene = s->scene;

  nvgTextAlign(s->vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);

    int btn_w = 150;
    int btn_h = 150;
    int btn_x = 1530;// 1920 - btn_w;
    int btn_y = 1080 - btn_h;


    if( touched && screen_button_clicked(touch_x, touch_y, btn_x, btn_y, 100, 100) )
    {
      scene.dash_menu_no++;
      if( scene.dash_menu_no > 2 )
         scene.dash_menu_no = 0;
    }
    

    nvgBeginPath(s->vg);
    nvgRoundedRect(s->vg, btn_x - 110, btn_y - 45, btn_w, btn_h, 100);
    nvgStrokeColor(s->vg, nvgRGBA(255, 255, 255, 80));
    nvgStrokeWidth(s->vg, 6);
    nvgStroke(s->vg);
    nvgFontSize(s->vg, 60*fFontSize);


    NVGcolor fillColor = nvgRGBA(0, 0, 255, 150);
    if( scene.dash_menu_no == 0)
    {
        fillColor = nvgRGBA(0, 0, 255, 50);
    }
    else
    {
        fillColor = nvgRGBA(0, 0, 255, 250);
    }

    nvgFillColor(s->vg, fillColor);
    nvgFill(s->vg);
    nvgFillColor(s->vg, nvgRGBA(255, 255, 255, 200));


    char  szText[50];
    sprintf( szText, "%d", scene.dash_menu_no );
    nvgText(s->vg, btn_x - 50, btn_y + 50, szText, NULL);

    if( scene.dash_menu_no == 2 )
    {
      const int bb_dmr_w = 300;
      const int bb_dmr_x = s->viz_rect.x + (s->viz_rect.w * 0.9) - bb_dmr_w - (bdr_s * 2);
      const int bb_dmr_y = (bdr_s + (bdr_s * 1.5)) + 220;

      draw_menu( s, bb_dmr_x, bb_dmr_y, bb_dmr_w );
      if( touched && screen_button_clicked(touch_x, touch_y, bb_dmr_x, bb_dmr_y, bb_dmr_w, 100) )
      { 
        
         scene.dash_menu_no = 0;
         printf("git pull\n");
         system("git pull");
      }
    }
}


static void ui_draw_debug(UIState *s) 
{
  UIScene &scene = s->scene;

  if( scene.dash_menu_no == 0 ) return;
  


  int  ui_viz_rx = s->viz_rect.x;
  int  y_pos = ui_viz_rx + 300;
  int  x_pos = 100+250; 

  float  steerRatio = scene.liveParameters.getSteerRatio();
  float  steerRatioCV = scene.liveParameters.getSteerRatioCV();
  float  steerActuatorDelayCV =  scene.liveParameters.getSteerActuatorDelayCV();
  float  fanSpeed = scene.deviceState.getFanSpeedPercentDesired();


  float  angleOffset = scene.liveParameters.getAngleOffsetDeg();
  float  angleOffsetAverage = scene.liveParameters.getAngleOffsetAverageDeg();
  float  stiffnessFactor = scene.liveParameters.getStiffnessFactor();

  float  modelSpeed = scene.car_state.getModelSpeed();

  float  laneWidth = scene.lateralPlan.getLaneWidth();
  //float  cpuPerc = scene.deviceState.getCpuUsagePercent();


  auto lane_line_probs = scene.modelDataV2.getLaneLineProbs();

    nvgTextAlign(s->vg, NVG_ALIGN_LEFT | NVG_ALIGN_BASELINE);
    nvgFontSize(s->vg, 36*1.5*fFontSize);

    //ui_print( s, ui_viz_rx+10, 50, "S:%d",  s->awake_timeout );

    x_pos = ui_viz_rx + 300;
    y_pos = 100+250; 

    ui_print( s, x_pos, y_pos+0,   "sR:%.2f, %.2f %.2f", steerRatio,  steerRatioCV, steerActuatorDelayCV );
    ui_print( s, x_pos, y_pos+50,  "aO:%.2f, %.2f", angleOffset, angleOffsetAverage );
    ui_print( s, x_pos, y_pos+100, "sF:%.2f Fan:%.0f", stiffnessFactor, fanSpeed/1000. );
    ui_print( s, x_pos, y_pos+150, "lW:%.2f CV:%.0f", laneWidth, modelSpeed );

    ui_print( s, x_pos, y_pos+250, "prob:%.2f, %.2f, %.2f, %.2f", lane_line_probs[0], lane_line_probs[1], lane_line_probs[2], lane_line_probs[3] );


    

    //float  dPoly = scene.pathPlan.lPoly + scene.pathPlan.rPoly;
    //ui_print( s, x_pos, y_pos+300, "Poly:%.2f, %.2f = %.2f", scene.pathPlan.lPoly, scene.pathPlan.rPoly, dPoly );
  // ui_print( s, x_pos, y_pos+350, "map:%d,cam:%d", scene.live.map_valid, scene.live.speedlimitahead_valid  );


    // tpms
    auto tpms = scene.car_state.getTpms();
    float fl = tpms.getFl();
    float fr = tpms.getFr();
    float rl = tpms.getRl();
    float rr = tpms.getRr();
    ui_print( s, x_pos, y_pos+350, "tpms:%.0f,%.0f,%.0f,%.0f", fl, fr, rl, rr );


   // int  lensPos = scene.frame.getLensPos();
   // int  lensTruePos = scene.frame.getLensTruePos();
    //int  lensErr = scene.frame.getLensErr();
  //  ui_print( s, x_pos, y_pos+400, "frame:%d,%d", lensPos, lensTruePos );



    ui_print( s, 0, 1020, "%s", scene.alert.text1 );
    ui_print( s, 0, 1078, "%s", scene.alert.text2 );

}

/*
  park @1;
  drive @2;
  neutral @3;
  reverse @4;
  sport @5;
  low @6;
  brake @7;
  eco @8;
*/
void ui_draw_gear( UIState *s )
{
  UIScene &scene = s->scene;
  NVGcolor nColor = COLOR_WHITE;

  cereal::CarState::GearShifter  getGearShifter = scene.car_state.getGearShifter();

  int  ngetGearShifter = int(getGearShifter);
  int  x_pos = 1700;
  int  y_pos = 200;
  char str_msg[512];

  nvgFontSize(s->vg, 150 );
  switch( ngetGearShifter )
  {
    case 1 : strcpy( str_msg, "P" ); nColor = nvgRGBA(200, 200, 255, 255); break;
    case 2 : strcpy( str_msg, "D" ); nColor = nvgRGBA(200, 200, 255, 255); break;
    case 3 : strcpy( str_msg, "N" ); nColor = COLOR_WHITE; break;
    case 4 : strcpy( str_msg, "R" ); nColor = COLOR_RED;  break;
    case 7 : strcpy( str_msg, "B" ); break;
    default: sprintf( str_msg, "-" ); break;
  }

  nvgFillColor(s->vg, nColor);
  ui_print( s, x_pos, y_pos, str_msg );
}


void update_dashcam(UIState *s)
{
  if (!s->awake) return;
  int touch_x = s->scene.mouse.touch_x;
  int touch_y = s->scene.mouse.touch_y;
  int touched = s->scene.mouse.touched;
  //int touch_cnt = s->scene.mouse.touch_cnt;

  if ( touched  ) 
  {
    s->scene.mouse.touched = 0;    
    printf("touched x,y: (%d,%d) %d  %d\n", touch_x, touch_y, touched, s->sidebar_collapsed);

    printf(" %d, %ld  %d \n", s->scene.started,  s->scene.started_frame,  s->vipc_client->connected);

  }

  if (!s->scene.started) return;
  if (s->scene.driver_view) return;

  screen_draw_button(s, touch_x, touch_y);
  screen_menu_button(s, touch_x, touch_y, touched);
  if (screen_button_clicked(touch_x, touch_y, 1700, 1000, 100, 100) )
  {
    click_elapsed_time = get_time() - click_time;

    printf( "screen_button_clicked %d  captureState = %d \n", click_elapsed_time, captureState );
    if (click_elapsed_time > 0)
    {
      click_time = get_time() + 1;
      screen_toggle_record_state();
    }
  }

 

  if( lock_current_video == true  )
  {
    float v_ego = s->scene.car_state.getVEgo();
    int engaged = s->scene.controls_state.getEngageable();
    if(  (v_ego < 0.1 || !engaged) )
    {
      elapsed_time = get_time() - stop_time;
      if( captureState == CAPTURE_STATE_CAPTURING && elapsed_time > 2 )
      {
        capture_cnt = 0;
        stop_capture();
      }
    }    
    else if( captureState != CAPTURE_STATE_CAPTURING )
    {
      capture_cnt = 0;
      start_capture();
    }
    else
    {
      stop_time = get_time();
    }
    
  }
  else  if( captureState == CAPTURE_STATE_CAPTURING )
  {
    capture_cnt = 0;
    stop_capture();
  }
  
 
  ui_draw_debug( s ); 
}





