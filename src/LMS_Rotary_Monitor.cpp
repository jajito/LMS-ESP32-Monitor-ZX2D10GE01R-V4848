/* Logitech Media Server REMOTE AND WEATHER CLOCK
with the ZX2D10GE01R-V4848

By : Jajito

https://github.com/jajito/LMS-ESP32-Monitor-ZX2D10GE01R-V4848-.git

Thanks to:

https://github.com/wireless-tag-com/ZX2D10GE01R-V4848.git
https://github.com/moononournation/Arduino_GFX.git
https://github.com/moononournation/LVGL_Music_Player.git
https://github.com/0015/ThatProject/tree/master/ESP32_LVGL/LVGL8/ZX2D10GE01R-V4848_Arduino
https://github.com/bitbank2/JPEGDEC.git
Icons:
https://www.dovora.com/
*/

#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include "button.hpp"
#include "mt8901.hpp"
#include "ui.h"
#include "FastLED.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "JPEGDEC.h"
#include <Arduino.h>
#include "time.h"
#include <Preferences.h>

//customization 
const char* ssid = "WIFI-SSID"; // put your wifi ssid here
const char* password = "WIFI-PASSWORD"; // put your wifi password here
String lmsIP = "192.168.1.33"; // PUT YOUR LMS server IP
String lmsPort = "9000"; // LMS port
// put your openWeatherManApiKey here
String openWeatherMapApiKey = "put your openWeatherManApiKey here";
// Replace with your country code and city
String city = "Marbella";
String countryCode = "ES";


Preferences preferences;

JPEGDEC jpeg;

#define ECO_O(y) (y > 0) ? -1 : 1
#define ECO_STEP(x) x ? ECO_O(x) : 0
#define GFX_BL 38
// Return the minimum of two values a and b
#define minimum(a,b)     (((a) < (b)) ? (a) : (b))

#define JPEG_FILENAME "/cover_lms.jpg"

Arduino_DataBus *bus = new Arduino_SWSPI(
  GFX_NOT_DEFINED /* DC */, 21 /* CS */,
  47 /* SCK */, 41 /* MOSI */, GFX_NOT_DEFINED /* MISO */);
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
  39 /* DE */, 48 /* VSYNC */, 40 /* HSYNC */, 45 /* PCLK */,
  10 /* R0 */, 16 /* R1 */, 9 /* R2 */, 15 /* R3 */, 46 /* R4 */,
  8 /* G0 */, 13 /* G1 */, 18 /* G2 */, 12 /* G3 */, 11 /* G4 */, 17 /* G5 */,
  47 /* B0 */, 41 /* B1 */, 0 /* B2 */, 42 /* B3 */, 14 /* B4 */,
  1 /* hsync_polarity */, 10 /* hsync_front_porch */, 10 /* hsync_pulse_width */, 10 /* hsync_back_porch */,
  1 /* vsync_polarity */, 14 /* vsync_front_porch */, 2 /* vsync_pulse_width */, 12 /* vsync_back_porch */);
Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
  480 /* width */, 480 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
  bus, GFX_NOT_DEFINED /* RST */, st7701_type7_init_operations, sizeof(st7701_type7_init_operations));

#define MOTOR_PIN 7 // pin for the buzzer
#define LED_PIN 4 // led pin
#define NUM_LEDS 13 // number of leds
CRGB leds[NUM_LEDS];

HTTPClient http;

static int16_t coverImgBitmapW;
static int16_t coverImgBitmapH;
static button_t *g_btn;
static lv_obj_t * currentButton = NULL;
static uint32_t screenWidth;
static uint32_t screenHeight;
static size_t coverImgBitmapSize = 0;
static uint8_t *coverImgBitmap;
static size_t IconImgBitmapSize = 0;
static uint8_t *IconImgBitmap;
static size_t thumbImgBitmapSize=0;
static uint8_t *thumbImgBitmap;

static lv_img_dsc_t img_cover;
static lv_img_dsc_t img_icon;
static lv_img_dsc_t img_thumb[10];

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;
static lv_disp_drv_t disp_drv;
static lv_group_t *lv_group;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600 *1;
const int   daylightOffset_sec = 3600 *1;

// weather stuff

String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&units=metric&lang=es&appid=" + openWeatherMapApiKey;//change language or metric if neccesary
String jsonBuffer;

uint16_t hue_led =0;
// uint16_t hue_lvgl = 0;
int volumen_lvgl = 0;
int16_t cont_volumen = 0;
int16_t cont_list = 0;
int16_t cont_slow = 0;
int timer_flag = 0; // timer active
uint32_t timer_time = 0; // milis for timer
uint32_t timer_count = 0; // variable for timer
int bounce = 0;
int update_Check_Frec = 1000; // time to update lms server in miliseconds
int update_Check_Act = 0; // actual for updateFrec
int update_Weather_Frec = 15*60000; //time to update weather in minutes
int update_Weather_Act = 0; // actual for updateWeather
int update_OutNavi_Time = 6000;// time to exit navigation screen
int update_OutNavi_Count = 0; // actual for exit navigation screen
boolean OutNavi_flag = false; // flag to exit navigation
bool is_playing = false;//is playing or not
bool is_navigating = false; // is in navigation screen?
int actualSongId = 0; // actual song ID
int cover_size = 480;// cover size. Fills entire screen
//int cover_size = 288; // cover size. Only fills the center
int thumb_size = 60; // thumbnail size
boolean first_navi_screen = true;
boolean first_players_screen = true;


static int _x, _y, _x_bound, _y_bound;

int number_of_players =1;
String Players_Drop_list ="";
const char* PlayerId[8]; // maximum number of players, change if there are more
const char* PlayerName[8];

const int list_elements = 10;// number of songs in playlist. 
const char* PlayListTitle[list_elements];// 
int PlayListIndex[list_elements];
int PlayListID[list_elements];


String lmsPlayer;
bool is_there_player = false;
String url = "http://"+ lmsIP + ":" + lmsPort + "/jsonrpc.js";
int coverID = 0;

String lmsPlayerPlayList ="" ;
// LMS codes
String lmsPlayerStatus = "[\"status\",\"-\"]"; // player status
String lms_server_image= "http://" + lmsIP + ":" + lmsPort + "/music/current/cover_" + cover_size + "x" + cover_size + ".jpg?player=" ;
String lms_server_id_image= "http://" + lmsIP + ":" + lmsPort + "/music/" + coverID +"/cover_" + thumb_size + "x" + thumb_size + ".jpg?player=" ;
String lmsNextSong = "[\"playlist\",\"index\",\"+1\"]";// next song
String lmsPreviousSong = "[\"playlist\",\"index\",\"-1\"]";// previous song
String lmsThisSong = "[\"playlist\",\"index\",\"+0\"]";// reinicia la cancion?
String lmsPlaySong = "[\"play\"]";// play song
String lmsStopSong = "[\"stop\"]";// stop song
String lmsPauseSong = "[\"pause\"]";// pause song
String lmsVolume = "[\"mixer\",\"volume\",\"<i>\"]"; // volume
String ListNumber = "0";
String lmsPlayNumberSong = "[\"playlist\",\"index\",\"" + ListNumber +"\"]";// proxima cancion

void lv_example_get_started_1(void);
void init_lv_group();
void encoder_read(lv_indev_drv_t *drv, lv_indev_data_t *data);
void zumba (int tiempo);
void wifi_begin(void);
void Get_Number_of_Players(void);
void Update_Players(void);
void Check_Regular();
void CheckUpdates(void);
void Check_Song_Info( int songId);
void update_cover(void);
String quita_acentos(String input_txt);
void printLocalTime();
void updateLocalTime();
void Get_Weather();
String httpGETRequest(const char* serverName);
//bool PrintIcon_mem(String url);

void create_players_select();

bool is_long_press = false;
bool PrintFile_mem(String url);
bool PrintFile_to_obj(String url, lv_obj_t *thumb_img_lvgl, int actual_thumb );
bool SendCommand(String url, String command);
void force_refresh_screen();
void Get_Playlist();
static int32_t jpegSeekHttpStream(JPEGFILE *pFile, int32_t iPosition);
static int jpegDraw(bool useBigEndian,int x, int y, int widthLimit, int heightLimit);
static int jpegOpenHttpStreamWithBuffer(WiFiClient *http_stream, uint8_t *buf, int32_t dataSize, JPEG_DRAW_CALLBACK *jpegDrawCallback);
static int32_t readStream(WiFiClient *http_stream, uint8_t *pBuf, int32_t iLen);
static void jpegCloseHttpStream(void *pHandle);
static int jpegOpenHttpStream(WiFiClient *http_stream, int32_t dataSize, JPEG_DRAW_CALLBACK *jpegDrawCallback);
static int32_t jpegReadHttpStream(JPEGFILE *pFile, uint8_t *pBuf, int32_t iLen);

static lv_obj_t * ui_PanelNavi2;
static lv_obj_t * ui_PanelPlayers;
static void btn_event_cb(lv_event_t *e);
static void btn_event_players_cb(lv_event_t *e);
static void event_handler_options_cb(lv_event_t * e);
void lv_list_screen_navigate(int list_elements);
void lv_list_screen_player(void);

//declare weather thumbs
LV_IMG_DECLARE( ui_img_clear_png);
LV_IMG_DECLARE( ui_img_thunderstorm_png);
LV_IMG_DECLARE( ui_img_snow_png);
LV_IMG_DECLARE( ui_img_rain_png);
LV_IMG_DECLARE( ui_img_drizzle_png);
LV_IMG_DECLARE( ui_img_day_partial_cloud_png);
LV_IMG_DECLARE( ui_img_clouds_png);
LV_IMG_DECLARE( ui_img_atmosphere_png);

// pixel drawing callback
// pixel drawing callback moonournation
static int jpegDrawCallback(JPEGDRAW *pDraw)
{
  //Serial.printf("Draw pos = %d,%d. size = %d x %d\n", pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  gfx_draw_bitmap_to_framebuffer(
      pDraw->pPixels, pDraw->iWidth, pDraw->iHeight,
      (uint16_t *)coverImgBitmap, pDraw->x, pDraw->y, cover_size, cover_size);

  return 1;
}
// pixel drawing callback for the thumbnail
static int jpegDrawCallback_thumb(JPEGDRAW *pDraw)
{
  //Serial.printf("Draw pos = %d,%d. size = %d x %d\n", pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  gfx_draw_bitmap_to_framebuffer(
      pDraw->pPixels, pDraw->iWidth, pDraw->iHeight,
      (uint16_t *)thumbImgBitmap , pDraw->x, pDraw->y, thumb_size, thumb_size);

  return 1;
}

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
  gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

  lv_disp_flush_ready(disp);
}

void setup() {
  Serial.begin(115200);
  #ifdef GFX_EXTRA_PRE_INIT
    GFX_EXTRA_PRE_INIT();
  #endif
  gfx->begin();
  gfx->fillScreen(BLACK);
  pinMode(GFX_BL, OUTPUT);// turns on the screen
  digitalWrite(GFX_BL, HIGH);
  pinMode(MOTOR_PIN, OUTPUT);// setup motor's pin
  lv_init();
  Serial.println("Rotatory Logitech Media Server Monitor");
  Serial.println("By Jajito");
  
  // Hardware Button
  g_btn = button_attch(3, 0, 10);
  // Magnetic Encoder
  mt8901_init(5, 6);
  //init the leds
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  screenWidth = gfx->width();
  screenHeight = gfx->height();
  // Must to use PSRAM
  disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * screenWidth * 32, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!disp_draw_buf) {
    Serial.println("LVGL disp_draw_buf allocate failed!");
  } else {
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, screenWidth * 32);
    /* Initialize the display */
    lv_disp_drv_init(&disp_drv);
    /* Change the following line to your display resolution */
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
   
    /* Initialize the input device driver */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.read_cb = encoder_read;
    indev_drv.type = LV_INDEV_TYPE_ENCODER;
    lv_indev_drv_register(&indev_drv);
    init_lv_group();
    
  }
  gfx->fillScreen(BLACK);
  wifi_begin();
  ui_init();
  
  
  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
  //get weather
  Get_Weather();
  preferences.begin("player_id", false);// opens storage space
  lmsPlayer= preferences.getString("last_player_id","0");// check if previous player
  if (lmsPlayer == "0"){
    Serial.println ("No previous player, select player");
    //opens select player screen
    is_there_player= false;
    lv_disp_load_scr(ui_ScreenNavigate);// loads inicial navigate screen
    lv_list_screen_player();// loads screen to choose player
  } 
  else{
    Serial.print("Previous Player : ");// there is previous player in preferences
    Serial.println(lmsPlayer);
    Get_Number_of_Players();
    Update_Players();
    int i;
    for (i=0 ; i< number_of_players; i++){
      //PlayerName[i] = doc["result"]["players_loop"][i]["name"];
      //PlayerId[i] = doc["result"]["players_loop"][i]["playerid"];
      //Serial.println ("Player N " + String(i) + " -Name : " + PlayerName[i] + " -Id : " + PlayerId[i]);
      if ( lmsPlayer.equals(PlayerId[i])){
        Serial.print("Previous player found : ");
        Serial.println (PlayerName[i]);
        lv_label_set_text(ui_LabelActualPlayer, PlayerName[i]);
        is_there_player = true;
      }
    }
    if (is_there_player== false){
      Serial.println ("Previous player not present, select player");
      //opens select player screen
      is_there_player= false;
      lv_disp_load_scr(ui_ScreenNavigate);// loads inicial navigate screen
      lv_list_screen_player();//
    }
    else if (is_there_player== true){
      lv_disp_load_scr(ui_ScreenPlay);// loads inicial PLAYER screen
      //lv_disp_load_scr(ui_ScreenNavigate);// carga pantalla inicial navigate
      //lv_disp_load_scr(ui_ScreenClock);// carga pantalla inicial CLOCK
    }
  }
}

void loop() {
  lv_timer_handler(); /* let the GUI do its work */
  CheckUpdates();
  delay(5);
}

void CheckUpdates(void){//
  if (is_there_player == false){
    //Serial.println ("there is no player");
    return;
  }
  else{
  uint32_t t = millis();
  if(t > (update_Check_Act + update_Check_Frec)){  //If the update time has passed...
    update_Check_Act = millis(); //Update actual time
    //Serial.println("Update periodical");
    Check_Regular();
    updateLocalTime();
  }
  if(t > (update_Weather_Act + update_Weather_Frec)){  //If the update weather time has passed...
    update_Weather_Act = millis(); //Updates acutal time
    Serial.println("Update weather");
    Get_Weather();
  }

  if (OutNavi_flag == true){
    //Serial.println("Start outnavigation");
    //uint32_t t = millis();
    if(t > (update_OutNavi_Count + update_OutNavi_Time)){  //If the outnavigation time has passed...
      update_OutNavi_Count = millis(); //Updates actual time
      Serial.println("Exit Navigate");
      OutNavi_flag = false;
      lv_disp_load_scr(ui_ScreenPlay);// loads player screen
      lv_group_focus_obj(ui_ButtonScrPlay1);
    }
  }
}
}
//gets playlist from LMS server
void Get_Playlist(){
  Serial.println("Get playlist");
  // Check WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {
    //Serial.println("Regular Update from " + url);
    http.begin(url);
    String lmsjson = "{\"method\": \"slim.request\", \"params\": [\"" + lmsPlayer + "\", " + lmsPlayerStatus + "]}";
    //Serial.println (lmsjson);
    String payload = "{}";
    int httpCode = http.POST(lmsjson);
    if (httpCode > 0) {
      //Serial.print("HTTP Response code: ");
      //Serial.println(httpCode);
      payload = http.getString();
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpCode);
      return;
    }  
    Serial.print ("Payload : ");
    Serial.println (payload);
    // Stream& input;
    DynamicJsonDocument doc(6144);
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.print("deserializeJson get playlist () failed: ");
      Serial.println(error.c_str());
      return;
    }
    JsonObject result = doc["result"];
    int IDList = 0;// mi contador de id
    for (JsonObject result_playlist_loop_item : result["playlist_loop"].as<JsonArray>()) {
      const char* result_playlist_loop_item_title = result_playlist_loop_item["title"]; // "Goodbye Stranger", ...
      int result_playlist_loop_item_playlist_index = result_playlist_loop_item["playlist index"]; // 53, 54, ...
      int result_playlist_loop_item_id = result_playlist_loop_item["id"]; // "-105489712", ...
      //Serial.println(result_playlist_loop_item_id);
      //get rid of accents
      String title  = result_playlist_loop_item_title;
      //Serial.println (title);
      //String title_sin = quita_acentos(title);// to eliminate acents if the font doesn't have accents
      String title_sin = title;//if the font has accents
       
      //Serial.println (title_sin);
      const char *title_sin_char = title_sin.c_str();//convert to string to const char*
      char* c = (char*)malloc(strlen(title_sin_char)+1);
      strcpy(c,title_sin_char);
      PlayListTitle[IDList]= c;
      PlayListIndex[IDList]= result_playlist_loop_item_playlist_index;
      PlayListID[IDList] = result_playlist_loop_item_id;
      IDList +=1;
      //Serial.print(" IDList = ");
      //Serial.println( IDList);
      if (IDList >= list_elements){
        //Serial.println(" break IDList = " + IDList);
        break;
      }
    }

    int result_mixer_volume = result["mixer volume"]; // 98
    const char* result_current_title = result["current_title"]; // nullptr
    
    //print the playlist
    // for (int i=0 ; i<list_elements; i++){
    //  Serial.printf("Playlist listid %02d", i);
    //  Serial.print(" index ");
    //  Serial.print(PlayListIndex[i]);
    //  Serial.print(" ID ");
    //  Serial.print(PlayListID[i]);
    //  Serial.print(" title ");
    //  Serial.println(PlayListTitle[i]);
    //} 
  http.end();
  }
}
// regular check
void Check_Regular() {
  // Check WiFi connection
  int check_regular_uptd_time = millis();
  //lv_label_set_text(ui_LabelActualPlayer, "check regular");
  if ((WiFi.status() == WL_CONNECTED)) {
    //Serial.println("Regular Update from " + url);
    http.begin(url);
    String lmsjson = "{\"method\": \"slim.request\", \"params\": [\"" + lmsPlayer + "\", " + lmsPlayerStatus + "]}";
    //Serial.println (lmsjson);
    String payload = "{}";
    int httpCode = http.POST(lmsjson);
    if (httpCode > 0) {
      //Serial.print("HTTP Response code: ");
      //Serial.println(httpCode);
      payload = http.getString();
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpCode);
      return;
    }  
    //Serial.print ("Payload : ");
    //Serial.println (payload);
    DynamicJsonDocument doc(15288);
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.print(F("deserializeJson check regular() failed: "));
      Serial.println(error.f_str());
      return;
    }
    JsonObject result = doc["result"];
    int result_remoteMeta_id = result["remoteMeta"]["id"]; // 683
    const char* result_mode = result["mode"]; // "play"
    if (strcmp(result_mode ,"play")== 0){
      is_playing = true;
      //Serial.println("is_playing");
    }
    else {
      is_playing = false;
      //Serial.println("no_playing");
    }
    int result_mixer_volume = result["mixer volume"]; // 85
    double result_duration = result["duration"]; // 226.666
    double result_time = result["time"];
    int duration_percentage = int((result_time/result_duration) *100);
    int seconds_left = int(result_duration - result_time);
    int seconds_front = int(result_time);
  
    int s,m,h;
    int t = seconds_left;// put here the time to print
    s = t % 60;
    t = (t - s)/60;
    m = t % 60;
    t = (t - m)/60;
    h = t;
    //Serial.printf(" time %2i:%2i ", m , s);
    // Serial.print("Duration : ");
    // Serial.print(result_duration);
    // Serial.print(" time : ");
    // Serial.print(result_time);
    // Serial.print(" % ");
    // Serial.println(duration_percentage);
    // // convert seconds to printable time
    lv_label_set_text_fmt(ui_LabelTimer, "-%d:%02d",m,s);
    lv_slider_set_value(ui_TimeSlider, duration_percentage, LV_ANIM_ON);
    //lv_label_set_text_fmt(ui_LabelActualPlayer, "duration : %i",result_duration );
    volumen_lvgl = result_mixer_volume;
    lv_arc_set_value(ui_ArcVolumen, result_mixer_volume);//updates volume in lvgl
    
    if (result_remoteMeta_id == actualSongId){// updates only basic stuff
        // updates volume and duration
        //Serial.print ("Volume : ");
        //Serial.println (result_mixer_volume);
        //lv_arc_set_value(ui_ArcVolumen, result_mixer_volume);
                
        if (is_playing == true){
          lv_label_set_text(ui_LabelPlayingInfo, "Playing");
          hue_led = ((volumen_lvgl * 255)/100);// reflects volume to the FASTLED  0-255
          //Serial.println (hue_led);
          FastLED.showColor(CHSV(hue_led, 255, 255));   
        }
        else{
          lv_label_set_text(ui_LabelPlayingInfo, "Paused");
          FastLED.showColor(CHSV(0, 0, 0));
          lv_disp_load_scr(ui_ScreenClock);// loads clock screen
          //lv_obj_set_style_border_color(ui_Button3, lv_color_hex(0x999999), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
      return;
    }    
    
    // updates song and cover
    Check_Song_Info(result_remoteMeta_id);
    update_cover();
    force_refresh_screen();
    if (first_navi_screen == false){
      Serial.println("delete previous navigation list");
      lv_obj_del(ui_PanelNavi2);
    }
    lv_list_screen_navigate(list_elements); //creates song list
    actualSongId = result_remoteMeta_id;
    http.end();
  }
  
}
// check song info
void Check_Song_Info( int songId) {
  // Check WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {
    //Serial.println("Check Song info Update from " + url);
    http.begin(url);
    String lmsjson = "{\"method\": \"slim.request\", \"params\": [\"" + lmsPlayer + "\", [\"songinfo\",\"-\",100, \"track_id:" + songId +"\"]]}";
    //Serial.println (lmsjson);
    String payload = "{}";
    int httpCode = http.POST(lmsjson);
    if (httpCode > 0) {
      //Serial.print("HTTP Response code: ");
      //Serial.println(httpCode);
      payload = http.getString();
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpCode);
      return;
    }  
    //Serial.print ("Payload : ");
    //Serial.println (payload);
    StaticJsonDocument<3048> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.print("deserializeJson song info ()  failed: ");
      Serial.println(error.c_str());
      return;
    }
    JsonArray result_songinfo_loop = doc["result"]["songinfo_loop"];
    const char* title = result_songinfo_loop[1]["title"]; // "La Noche Que la Luna ...
    const char* artist = result_songinfo_loop[2]["artist"];
    const char* album = result_songinfo_loop[10]["album"]; // sometimes LMS changes the number...
    String separador = " - ";
    String c  = artist + separador + title + separador + album ;
    //Serial.printf("json result : %c\n", c);
    //Serial.println (c);
    // to eliminate accents if font doesn' have accents
    // String c_spa = c.substring(0,100);
    // c_spa.replace("á", "a");
    // c_spa.replace("é", "e");
    // c_spa.replace("í", "i");
    // c_spa.replace("ó", "o");
    // c_spa.replace("ú", "u");
    // c_spa.replace("Á", "A");
    // c_spa.replace("É", "E");
    // c_spa.replace("Í", "I");
    // c_spa.replace("Ó", "O");
    // c_spa.replace("Ú", "U");
    // c_spa.replace("ñ", "ni");
    // c_spa.replace("Ñ", "Ni");
    
    //Serial.printf("Song info : %c\n", c_spa);
    String c_spa = c;
    Serial.println (c_spa);
    const char *song_info_txt = c_spa.c_str(); 
    lv_label_set_text( ui_LabelSongInfoPlay, song_info_txt);
    http.end();
  }
}
//eliminate accents in a string
String quita_acentos(String input_txt){
  String c_spa = input_txt.substring(0,100);
  c_spa.replace("á", "a");
  c_spa.replace("é", "e");
  c_spa.replace("í", "i");
  c_spa.replace("ó", "o");
  c_spa.replace("ú", "u");
  c_spa.replace("Á", "A");
  c_spa.replace("É", "E");
  c_spa.replace("Í", "I");
  c_spa.replace("Ó", "O");
  c_spa.replace("Ú", "U");
  c_spa.replace("ñ", "ni");
  c_spa.replace("Ñ", "Ni");
  return c_spa;
}
// update cover
void update_cover(void){
  bool printed_file_mem_ok = PrintFile_mem(lms_server_image + lmsPlayer);
}

// starts lvgls group
void init_lv_group() {
  lv_group = lv_group_create();
  lv_group_set_default(lv_group);
  lv_indev_t *cur_drv = NULL;
  for (;;) {
    cur_drv = lv_indev_get_next(cur_drv);
    if (!cur_drv) {
      break;
    }
    if (cur_drv->driver->type == LV_INDEV_TYPE_ENCODER) {
      Serial.println("Encoder linked to group");
      lv_indev_set_group(cur_drv, lv_group);
    }
  }
}

// read encoder
void encoder_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  if (lv_scr_act() ==ui_ScreenPlay){//Player Screen
    static int16_t cont_volumen_last = 0;
    cont_volumen = mt8901_get_count();
    
    if (cont_volumen != cont_volumen_last){
      int16_t dif_volumen = cont_volumen_last - cont_volumen;
      //Serial.printf("dif volumen : %02d. cont volumen : %02d\n", dif_volumen, cont_volumen);
      volumen_lvgl = min<int>(max<int>(volumen_lvgl + dif_volumen, 0), 100);//
      //Serial.printf( "volumen lvgl : %02d\n" , volumen_lvgl);
      lv_arc_set_value(ui_ArcVolumen , volumen_lvgl);
      lv_label_set_text_fmt (ui_LabelPlayingInfo, "Vol: %02d", volumen_lvgl);
      hue_led = ((volumen_lvgl * 255)/100);// adapts volume to fastled FASTLED 0-255
      //Serial.println (hue_led);
      FastLED.showColor(CHSV(hue_led, 255, 255));    
      // sends volume change to LMS
      char buf[50];
      int i = volumen_lvgl;
      char* lmsVolume = "[\"mixer\",\"volume\",\"%i\"]";
      sprintf(buf, lmsVolume, i);
      //Serial.println (buf);
      String lmsjson = "{\"method\": \"slim.request\", \"params\": [\"" + lmsPlayer + "\", " + buf + "]}";
      SendCommand (url, lmsjson);
    }
    cont_volumen_last = cont_volumen;
    data->btn_id = 0;// this selects a concrete button
    
    if (button_isPressed(g_btn)) {
      Serial.println("boton pressed");
      data->state = LV_INDEV_STATE_PR;
    } else {
      data->state = LV_INDEV_STATE_REL;
    }
          
    if (button_wasPressFor (g_btn, 500)){
      Serial.println("boton long pressed");
      // do the stuff when long pressed
      is_long_press = true;
    }
  }
  else if (lv_scr_act() ==ui_ScreenNavigate){//Navigator Screen LMS
    static int16_t cont_last = 0;
    int16_t cont_now = mt8901_get_count();
    //Serial.println(cont_now);
    data->enc_diff = ECO_STEP(cont_now - cont_last);
    //Serial.print("Encoder read : ");
    //Serial.println(ECO_STEP(cont_now - cont_last));
    if (cont_last != cont_now){
      //Serial.println("encoder changed");
      update_OutNavi_Count = millis();
      OutNavi_flag = true;// starts outnavi counter
    }
    
    cont_last = cont_now;
    if (button_isPressed(g_btn)) {
      data->state = LV_INDEV_STATE_PR;
    } else {
      data->state = LV_INDEV_STATE_REL;
    }
    
  }
  else{
    if (button_isPressed(g_btn)) {
      lv_disp_load_scr(ui_ScreenPlay);
    }
  }
}

// buzzer
void zumba (int tiempo){
  digitalWrite(MOTOR_PIN, HIGH);
  delay(tiempo);
  digitalWrite(MOTOR_PIN, LOW);
}

void wifi_begin(void){
WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}


void force_refresh_screen()
{
  Serial.println("force refresh lvgl");
  lv_obj_invalidate(ui_ScreenPlay);
  lv_obj_invalidate(ui_ImageCover);
  lv_timer_handler(); /* let the GUI do its work */
}

// send commands to LMS
bool SendCommand(String url, String command) {
  //Serial.println("Sending Command "  + command + " to " + url);
  // Check WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {
    //PRINT_STR("[HTTP] begin...\n");
  //HTTPClient http;
  // Configure server and url
  http.begin(url);
  int httpCode = http.POST(command);
  if (httpCode > 0) {
    //Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    // File found at server
    if (httpCode == HTTP_CODE_OK) {
       //Serial.println ("Command sent");
    }
    else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      //PRINT_STR(" [HTTP] GET... failed, error");
      return 0;
    }
    //http.end();
  }
  String s = http.getString();//to avoid error "no more processes"
  //Serial.print(s + "\n");
  http.end();
  return 1; // File was fetched from web
}
}
//gets number of players
void Get_Number_of_Players(void){
// Check WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {
    //Serial.println("Get number of players from " + url);
    http.begin(url);
    String lmsjson = "{\"method\": \"slim.request\", \"params\": [\"FF:FF:FF:FF\", [\"player\",\"count\",\"?\"]]}";
    //Serial.println (lmsjson);
    String payload = "{}";
    int httpCode = http.POST(lmsjson);
    if (httpCode > 0) {
      //Serial.print("HTTP Response code: ");
      //Serial.println(httpCode);
      payload = http.getString();
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpCode);
      return;
    }  
    //Serial.print ("Payload : ");
    //Serial.println (payload);
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.print("deserializeJson number of players() failed: ");
      Serial.println(error.c_str());
      return;
    }
    number_of_players = doc["result"]["_count"];
    Serial.println ("Number of players : " + String(number_of_players));
  } 
}
//get players info
void Update_Players(void){ 
  if ((WiFi.status() == WL_CONNECTED)) {
    //Serial.println("Get players from " + url);
    http.begin(url);
    String lmsjson = "{\"method\": \"slim.request\", \"params\": [\"FF:FF:FF:FF\", [\"players\",\"0\",\"" + String(number_of_players) + "\"]]}";
    //Serial.println (lmsjson);
    String payload = "{}";
    int httpCode = http.POST(lmsjson);
    if (httpCode > 0) {
    //Serial.print("HTTP Response code: ");
    //Serial.println(httpCode);
      payload = http.getString();
    }
    else {
      Serial.print("Error code: ");
      Serial.println(httpCode);
      return;
    }  
    //Serial.print ("Payload Players : ");
    //Serial.println (payload);
    StaticJsonDocument<2500> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        Serial.print("deserializeJson update players() failed: ");
        Serial.println(error.c_str());
        return;
    }
     //char PlayerId[number_of_players][17];
    //PlayerId[number_of_players][17];
     static char opts[256] = {""};
    int i;
    for (i=0 ; i< number_of_players; i++){
      PlayerName[i] = doc["result"]["players_loop"][i]["name"];
      PlayerId[i] = doc["result"]["players_loop"][i]["playerid"];
      Serial.println ("Player N " + String(i) + " -Name : " + PlayerName[i] + " -Id : " + PlayerId[i]);
      
    }
    Serial.println (opts); 
  }
}

// creates playlist screen
void lv_list_screen_navigate(int list_elements){
    int navi_screen_uptd_time = millis();
    Get_Playlist();
    /*Create a list*/
    ui_PanelNavi2 = lv_list_create(ui_ScreenNavigate);
    //listFiles = lv_list_create(ui_ScreenNavigate());
    lv_label_set_text(ui_LabelScreenNavigate, "Play List" );
    
    lv_obj_set_width(ui_PanelNavi2, 340);
    lv_obj_set_height(ui_PanelNavi2, 340);
    lv_obj_set_x(ui_PanelNavi2, 0);
    lv_obj_set_y(ui_PanelNavi2, 0);
    lv_obj_set_align(ui_PanelNavi2, LV_ALIGN_CENTER);
    //lv_obj_clear_flag(ui_PanelNavi2, LV_OBJ_FLAG_SCROLLABLE);// take off to get scroll
    lv_obj_set_style_radius(ui_PanelNavi2, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_PanelNavi2, lv_color_hex(0x474747), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_PanelNavi2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui_PanelNavi2, lv_color_hex(0x0E0D0D), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui_PanelNavi2, LV_GRAD_DIR_HOR, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_PanelNavi2, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_PanelNavi2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_pad_all ( ui_PanelNavi2, 1, LV_PART_MAIN | LV_STATE_DEFAULT);//quitar márgenes?
    lv_obj_set_style_text_font(ui_PanelNavi2, &ui_font_opensansreg22, LV_PART_MAIN | LV_STATE_DEFAULT);
    // creatres a playlist title
    lv_list_add_text(ui_PanelNavi2, "Play List");

    /*Add buttons to the list*/
    lv_obj_t * btn;
    int i;
    for(i = 0; i < list_elements; i++) {
        btn = lv_btn_create(ui_PanelNavi2);
        lv_obj_set_width(btn, lv_pct(100));//era 50
        lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_set_width(btn, 340);
        lv_obj_set_height(btn, 60);
        lv_obj_set_style_radius(btn, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(btn, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT); 
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_FOCUSED);// CAMBIA EL COLOR SI FOCUS
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x474545), LV_PART_MAIN | LV_STATE_DEFAULT);// COLOR
        
        lv_obj_t * lab1 = lv_label_create(btn);
        lv_label_set_text_fmt(lab1, "%d", i);// creates a label with song's number in the list

        lv_obj_t * lab2 = lv_label_create(btn);
        //lv_label_set_text_fmt(lab, "Item %d", i);
        lv_obj_set_width(lab2, 244);
        lv_obj_set_height(lab2, LV_SIZE_CONTENT);    /// 1
        lv_obj_set_x(lab2, 59);
        lv_obj_set_y(lab2, -8); 
        lv_label_set_text(lab2, "TextA");
        
        lv_obj_set_style_text_color(lab2, lv_color_hex(0xCFCECE), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(lab2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lab2, &ui_font_opensansreg22, LV_PART_MAIN | LV_STATE_DEFAULT); 
        
        lv_label_set_text(lab2, PlayListTitle[i]);
        //Serial.print("playlist title nº : ");
        //Serial.print( i );
        //Serial.println(PlayListTitle[i]);

        lv_obj_t * img = lv_img_create(btn);
        lv_obj_set_width(img, 60);
        lv_obj_set_height(img, 60);
        lv_obj_set_x(img, -17);
        lv_obj_set_y(img, -18);
        lv_obj_set_flex_flow(img, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(img, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        //get thumb of the song in the list
        coverID = PlayListID[i];
        lms_server_id_image= "http://" + lmsIP + ":" + lmsPort + "/music/" + coverID +"/cover_" + thumb_size + "x" + thumb_size + ".jpg?player=" ;
        PrintFile_to_obj(lms_server_id_image, img, i);
        // move focus to navigation panel
        lv_group_focus_obj(ui_PanelNavi2);
        /*Select the first button by default*/
        currentButton = lv_obj_get_child(ui_PanelNavi2, 1);
        lv_obj_add_state(currentButton, LV_STATE_FOCUSED);
        //refresh LVGL after each thumb
        lv_timer_handler();
    }
    //lv_list_add_text(ui_PanelNavi2, "Options");// adds options button
    btn = lv_btn_create(ui_PanelNavi2);
    //btn = lv_list_add_btn(ui_PanelNavi2, LV_SYMBOL_SETTINGS, "Choose Player");
    
    lv_obj_add_event_cb(btn, event_handler_options_cb, LV_EVENT_CLICKED, NULL);//(btn, event_handler, LV_EVENT_CLICKED, NULL);
    lv_obj_set_width(btn, 340);
    lv_obj_set_height(btn, 60);
    lv_obj_set_style_radius(btn, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT); 
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_FOCUSED);// CAMBIA EL COLOR SI FOCUS
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x474545), LV_PART_MAIN | LV_STATE_DEFAULT);// COLOR
    
    lv_obj_t * lab2 = lv_label_create(btn);
    //lv_label_set_text_fmt(lab, "Item %d", i);
    lv_obj_set_width(lab2, 244);
    lv_obj_set_height(lab2, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(lab2, 59);
    lv_obj_set_y(lab2, -8); 
    lv_label_set_text(lab2, "SELECT PLAYER...");
        
    lv_obj_set_style_text_color(lab2, lv_color_hex(0xCFCECE), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(lab2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(lab2, &ui_font_opensansreg22, LV_PART_MAIN | LV_STATE_DEFAULT); 
  
  first_navi_screen = false;
  
  Serial.print("Time to update navigate screen : ");
  Serial.println(millis()- navi_screen_uptd_time);
}

// options button callback
static void event_handler_options_cb(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t * obj = lv_event_get_target(e);
  if(code == LV_EVENT_CLICKED) {
      Serial.println("callback options");
      lv_list_screen_player(); //create players list
    }
}

// creates screen to choose player
void lv_list_screen_player(void){
    Get_Number_of_Players();
    Update_Players();
    if (is_there_player == true){
      Serial.println("delete navigation list before players list");
      lv_obj_del(ui_PanelNavi2);
    }
    int navi_screen_uptd_time = millis();
    lv_label_set_text(ui_LabelScreenNavigate, "Choose Player" );
    /*Create a list*/
    ui_PanelPlayers = lv_list_create(ui_ScreenNavigate);
    lv_obj_set_size (ui_PanelPlayers, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_PanelPlayers, 0);
    lv_obj_set_y(ui_PanelPlayers, 0);
    lv_obj_set_align(ui_PanelPlayers, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(ui_PanelPlayers, lv_color_hex(0x474747), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_PanelPlayers, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(ui_PanelPlayers, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_pad_all ( ui_PanelPlayers, 1, LV_PART_MAIN | LV_STATE_DEFAULT);//quitar márgenes?
    lv_obj_set_style_text_font(ui_PanelPlayers, &ui_font_opensansreg22, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    /*Add buttons to the list*/
    lv_obj_t * btn;
    int i;
    for(i = 0; i < number_of_players; i++) {
        btn = lv_btn_create(ui_PanelPlayers);
        lv_obj_set_width(btn, lv_pct(100));//era 50
        lv_obj_add_event_cb(btn, btn_event_players_cb, LV_EVENT_CLICKED, NULL);//(btn, event_handler, LV_EVENT_CLICKED, NULL);
        lv_obj_set_width(btn, 340);
        lv_obj_set_height(btn, 50);
        lv_obj_set_style_radius(btn, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(btn, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_opa(btn, 255, LV_PART_MAIN | LV_STATE_DEFAULT); 
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_FOCUSED);// CAMBIA EL COLOR SI FOCUS
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x474545), LV_PART_MAIN | LV_STATE_DEFAULT);// COLOR
        
        lv_obj_t * lab1 = lv_label_create(btn);
        lv_label_set_text_fmt(lab1, "%d", i);// crea una label con el numero del player
        lv_obj_add_flag(lab1, LV_OBJ_FLAG_HIDDEN);

        lv_obj_t * lab2 = lv_label_create(btn);
        //lv_label_set_text_fmt(lab, "Item %d", i);
        lv_obj_set_width(lab2, 340);
        lv_obj_set_height(lab2, LV_SIZE_CONTENT);    /// 1
        Serial.println(PlayerName[i]);
        lv_label_set_text(lab2, PlayerName[i]);
       
        lv_obj_set_style_text_color(lab2, lv_color_hex(0xCFCECE), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_opa(lab2, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(lab2, &ui_font_opensansreg22, LV_PART_MAIN | LV_STATE_DEFAULT); 
           
        // move focus to players panel
        lv_group_focus_obj(ui_PanelPlayers);
        /*Select the first button by default*/
        currentButton = lv_obj_get_child(ui_PanelPlayers, 0);
        lv_obj_add_state(currentButton, LV_STATE_FOCUSED);
    }
  first_players_screen = false;
  
  Serial.print("Time to update players screen : ");
  Serial.println(millis()- navi_screen_uptd_time);
}

// Play screen call back
void btn_scrPlay1_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *ui_ButtonScrPlay1 = lv_event_get_target(e);
  Serial.println("callback ui_ButtonScrPlay1");
  if (is_long_press==true){//
    Serial.println("was long press");
  
    // do the stuff when long pressed
    Serial.println("cambia a screennavigate");
    update_OutNavi_Count = millis();
    OutNavi_flag = true;// starts outnavi counter
    Serial.println ("out navi flag to true");
    lv_disp_load_scr(ui_ScreenNavigate);// loads screen
    is_long_press = false;
    return;
  }
  else{
    Serial.println("pause song");
    String lmsCommand = lmsPauseSong;
    String lmsjson = "{\"method\": \"slim.request\", \"params\": [\"" + lmsPlayer + "\", " + lmsCommand + "]}";
    SendCommand (url, lmsjson);
  }
}

//navigate screen call back
static void btn_event_cb(lv_event_t *e) {
  Serial.println("callback btn_cb_event");
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btn = lv_event_get_target(e);
  if (code == LV_EVENT_CLICKED) {
    if (btn == ui_ButtonScrPlay1) {
      Serial.println("Se ha pulsado el boton de la pantalla play desde el btn_event_cb");
      lv_disp_load_scr(ui_ScreenNavigate);// carga la screen de Navigate
    } else {
      Serial.println("Se ha pulsado otro boton de navigate");
    lv_obj_t *label = lv_obj_get_child(btn, 0);
    char *label_text = lv_label_get_text(label);// get the label text and print it
    Serial.println (label_text);
    const char *btn_txt = lv_list_get_btn_text(ui_PanelNavi2, btn);
    Serial.print("Boton txt : ");
    Serial.println (btn_txt);
    // plays the song in the button label
    //ListNumber = btn_txt;
    int btn_int = atoi(btn_txt);//convert txt to int
    ListNumber = PlayListIndex[btn_int];
    Serial.printf("Playing: listid %02d", btn_int);
      Serial.print(" index ");
      Serial.print(PlayListIndex[btn_int]);
      Serial.print(" ID ");
      Serial.print(PlayListID[btn_int]);
      Serial.print(" title ");
      Serial.println(PlayListTitle[btn_int]);
    lmsPlayNumberSong = "[\"playlist\",\"index\",\"" + ListNumber +"\"]";// next song
    String lmsCommand = lmsPlayNumberSong;// plays the song
    String lmsjson = "{\"method\": \"slim.request\", \"params\": [\"" + lmsPlayer + "\", " + lmsCommand + "]}";
    Serial.println(lmsjson);
    SendCommand (url, lmsjson);
    lv_disp_load_scr(ui_ScreenPlay);
    lv_group_focus_obj(ui_ButtonScrPlay1);
    force_refresh_screen();
    //update_cover();
    //force_refresh_screen();
    //Check_Regular();
    }
  }
}

//players screen call back
static void btn_event_players_cb(lv_event_t *e) {
  Serial.println("callback btn_cb_plasyers_event");
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *btn = lv_event_get_target(e);
  if (code == LV_EVENT_CLICKED) {
    if (btn == ui_ButtonScrPlay1) {
      Serial.println("Se ha pulsado el boton de la pantalla play desde el btn_event_cb");
      lv_disp_load_scr(ui_ScreenNavigate);// carga la screen de Navigate
    } else {
      Serial.println("Se ha pulsado otro boton de navigate en players");
      lv_obj_t *label = lv_obj_get_child(btn, 0);
      char *label_text = lv_label_get_text(label);// get the label text and print it
      Serial.println (label_text);
      const char *btn_txt = lv_list_get_btn_text(ui_PanelPlayers, btn);
      Serial.print("Boton txt : ");
      Serial.println (btn_txt);
      int btn_int = atoi(btn_txt);//convert txt to int
      Serial.print("Boton int : ");
      Serial.println(btn_int);
      Get_Number_of_Players();
      Update_Players();
      lmsPlayer = PlayerId[btn_int];
      Serial.print ("Selected lms player : ");
      Serial.println (lmsPlayer);
      Serial.print("Selected Players name : ");
      Serial.println (PlayerName[btn_int]);
    
      // store the selected Player in preferences
      //preferences.remove("last_player_id");//quitar, es para probar la deteccion de players
      //preferences.putString("last_player_id", "8c:4b:14:08:28:a5");//quitar es para probar la deteccion de players
      preferences.putString("last_player_id", lmsPlayer);
      is_there_player = true;
      lv_label_set_text(ui_LabelActualPlayer, PlayerName[btn_int]);
      lv_disp_load_scr(ui_ScreenPlay);// carga la screen de Play
      lv_group_focus_obj(ui_ButtonScrPlay1);
      lv_timer_handler(); /* let the GUI do its work */
      force_refresh_screen();
      lv_obj_del(ui_PanelPlayers);// delete panel players
      first_players_screen == true;
      lv_list_screen_navigate(list_elements); 
    }
  }
}

// pixel drawing callback de moonournation
static int jpegDrawCallbackGFX(JPEGDRAW *pDraw)// prints with gfx, noto with lvgl
{
  // Serial.printf("Draw pos = %d,%d. size = %d x %d\n", pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight);
  gfx->draw16bitRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
  return 1;
}

static int jpegOpenHttpStream(WiFiClient *http_stream, int32_t dataSize, JPEG_DRAW_CALLBACK *jpegDrawCallback)
{
  Serial.println("jpegOpenHttpStream");
  return jpeg.open(http_stream, dataSize, jpegCloseHttpStream, jpegReadHttpStream, jpegSeekHttpStream, jpegDrawCallback);
}

static int jpegOpenHttpStreamWithBuffer(WiFiClient *http_stream, uint8_t *buf, int32_t dataSize, JPEG_DRAW_CALLBACK *jpegDrawCallback)

{
  //Serial.println("jpegOpenHttpStreamWithBuffer");
  int32_t r = readStream(http_stream, buf, dataSize);
  if (r != dataSize)
  {
    return 0;
  }
  return jpeg.openRAM(buf, dataSize, jpegDrawCallback);
}

static void jpegCloseHttpStream(void *pHandle)
{
  // printf("jpegCloseHttpStream\n");
  // WiFiClient *http_stream = (WiFiClient *)pHandle;
  // do nothing
}
    
static int32_t jpegReadHttpStream(JPEGFILE *pFile, uint8_t *pBuf, int32_t iLen)
{
  // printf("jpegReadHttpStream, iLen: %d\n", iLen);
  WiFiClient *http_stream = (WiFiClient *)pFile->fHandle;
  return readStream(http_stream, pBuf, iLen);
}

static int32_t readStream(WiFiClient *http_stream, uint8_t *pBuf, int32_t iLen)
{
  uint8_t wait = 0;
  size_t a = http_stream->available();
  size_t r = 0;
  while ((r < iLen) && (wait < 10))
  {
    if (a)
    {
      wait = 0; // reset wait count once available
      r += http_stream->readBytes(pBuf + r, iLen - r);
      // printf("1st byte: %d, 2nd byte: %d, last byte: %d, iLen: %d, r: %d, wait: %d\n", pBuf[0], pBuf[1], pBuf[r - 1], iLen, r, wait);
    }
    else
    {
      delay(100);
      wait++;
    }
    a = http_stream->available();
  }
  return r;
}



static int jpegDraw(bool useBigEndian,
                    int x, int y, int widthLimit, int heightLimit)
{
  _x = x;
  _y = y;
  _x_bound = _x + widthLimit - 1;
  _y_bound = _y + heightLimit - 1;

  // scale to fit height
  int _scale;
  int iMaxMCUs;
  float ratio = (float)jpeg.getHeight() / heightLimit;
  if (ratio <= 1)
  {
    _scale = 0;
    iMaxMCUs = widthLimit / 16;
  }
  else if (ratio <= 2)
  {
    _scale = JPEG_SCALE_HALF;
    iMaxMCUs = widthLimit / 8;
  }
  else if (ratio <= 4)
  {
    _scale = JPEG_SCALE_QUARTER;
    iMaxMCUs = widthLimit / 4;
  }
  else
  {
    _scale = JPEG_SCALE_EIGHTH;
    iMaxMCUs = widthLimit / 2;
  }
  jpeg.setMaxOutputSize(iMaxMCUs);
  if (useBigEndian)
  {
    jpeg.setPixelType(RGB565_BIG_ENDIAN);
  }
  int decode_result = jpeg.decode(x, y, _scale);
  jpeg.close();

  return decode_result;
}

static int32_t jpegSeekHttpStream(JPEGFILE *pFile, int32_t iPosition)
{
  // printf("jpegSeekHttpStream, pFile->iPos: %d, iPosition: %d\n", pFile->iPos, iPosition);
  WiFiClient *http_stream = (WiFiClient *)pFile->fHandle;
  http_stream->readBytes((uint8_t *)nullptr, iPosition - pFile->iPos);
  return iPosition;
}


// downloads the imgage to mem wihtout buffer. moonournations
bool PrintFile_mem(String url) //funciona
{
  Serial.println("Downloading to mem no buffer to lvgl: " + url);
  // Check WiFi connection
  if ((WiFi.status() == WL_CONNECTED))
  {
    unsigned long start = millis();
    int jpeg_result = 0;
    http.begin(url);
    int httpCode = http.GET();
    //Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    if (httpCode <= 0)
      {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
    else
      {
        if (httpCode != 200)
        {
          Serial.printf("[HTTP] Not OKKKKKK!\n");
          //gfx->printf("[HTTP] Not OK!\n");
          delay(5000);
        }
        else
        {
          // get lenght of document(is - 1 when Server sends no Content - Length header)
          int len = http.getSize();
          Serial.printf("[HTTP] size: %d\n", len);
          //gfx->printf("[HTTP] size: %d\n", len);

          if (len <= 0)
          {
            Serial.printf("[HTTP] Unknow content size: %d\n", len);
            //gfx->printf("[HTTP] Unknow content size: %d\n", len);
          }
          else
          {
            uint8_t *buf = (uint8_t *)malloc(len);
            //Serial.println("buffer created");
            static WiFiClient *http_stream = http.getStreamPtr();
            jpeg_result = jpegOpenHttpStreamWithBuffer(http_stream, buf, len, jpegDrawCallback);//ponemos el drawback de moon y funciona
            if (jpeg_result)
            {
              int iWidth = jpeg.getWidth();
              int iHeight = jpeg.getHeight();
              Serial.println("Successfully opened JPEG image jpeg draw callback");
              Serial.printf("Image size: %d x %d, orientation: %d, bpp: %d\n", iWidth, iHeight,jpeg.getOrientation(), jpeg.getBpp());
              size_t imgBitmapSize = iWidth * iHeight * 2;
              coverImgBitmap = (uint8_t *)malloc(imgBitmapSize);
              coverImgBitmapSize = imgBitmapSize;
              jpeg.setPixelType(RGB565_BIG_ENDIAN);
              jpeg.decode(0, 0, 0);
              img_cover.header.cf = LV_IMG_CF_TRUE_COLOR;
              img_cover.header.w = iWidth;
              img_cover.header.h = iHeight;
              img_cover.header.always_zero = 0;
              img_cover.data_size = imgBitmapSize;
              img_cover.data = coverImgBitmap;
              lv_img_set_src(ui_ImageCover, &img_cover);
              lv_obj_clear_flag(ui_ImageCover, LV_OBJ_FLAG_HIDDEN);
              //cover_Animation(ui_ImageCover, 10);
              free(buf);
            }
          }
        }
        http.end();
      }
    Serial.printf("Time used printing from RAM with NO buf in lvgl: %lu\n", millis() - start);
    // notify WDT still working
    //feedLoopWDT();
    return 1; // File was fetched from web
  }
}
// downloads image to mem no buffer moonournations
bool PrintFile_to_obj(String url, lv_obj_t *thumb_img_lvgl, int actual_thumb )
{
  //Serial.println("Printing image to lvgl image object: " + url);
  // Check WiFi connection
  if ((WiFi.status() == WL_CONNECTED))
  {
    unsigned long start = millis();
    int jpeg_result = 0;
    http.begin(url);
    int httpCode = http.GET();
    //Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    if (httpCode <= 0)
      {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
    else
      {
        if (httpCode != 200)
        {
          Serial.printf("[HTTP] Not OKKKKKK!\n");
          //gfx->printf("[HTTP] Not OK!\n");
          delay(5000);
        }
        else
        {
          // get lenght of document(is - 1 when Server sends no Content - Length header)
          int len = http.getSize();
          //Serial.printf("[HTTP] size: %d\n", len);
          //gfx->printf("[HTTP] size: %d\n", len);

          if (len <= 0)
          {
            Serial.printf("[HTTP] Unknow content size: %d\n", len);
            //gfx->printf("[HTTP] Unknow content size: %d\n", len);
          }
          else
          {
            uint8_t *buf = (uint8_t *)malloc(len);
            //Serial.println("buffer created");
            static WiFiClient *http_stream = http.getStreamPtr();
            jpeg_result = jpegOpenHttpStreamWithBuffer(http_stream, buf, len, jpegDrawCallback_thumb);//ponemos el drawback de moon y funciona
            if (jpeg_result)
            {
              int iWidth = jpeg.getWidth();
              int iHeight = jpeg.getHeight();
              //Serial.println("Successfully opened JPEG image jpeg draw callback");
              //Serial.printf("Image size: %d x %d, orientation: %d, bpp: %d\n", iWidth, iHeight,jpeg.getOrientation(), jpeg.getBpp());
              size_t imgBitmapSize = iWidth * iHeight * 2;
              thumbImgBitmap = (uint8_t *)malloc(imgBitmapSize);
              thumbImgBitmapSize = imgBitmapSize;
              jpeg.setPixelType(RGB565_BIG_ENDIAN);
              jpeg.decode(0, 0, 0);
              img_thumb[actual_thumb].header.cf = LV_IMG_CF_TRUE_COLOR;
              img_thumb[actual_thumb].header.w = iWidth;
              img_thumb[actual_thumb].header.h = iHeight;
              img_thumb[actual_thumb].header.always_zero = 0;
              img_thumb[actual_thumb].data_size = imgBitmapSize;
              img_thumb[actual_thumb].data = thumbImgBitmap;
              lv_img_set_src(thumb_img_lvgl, &img_thumb[actual_thumb]);
              lv_obj_clear_flag(thumb_img_lvgl, LV_OBJ_FLAG_HIDDEN);
              //cover_Animation(ui_ImageCover, 10);
              free(buf);
            }
          }
        }
        http.end();
      }
    //Serial.printf("Time used printing from RAM with NO buf in lvgl: %lu\n", millis() - start);
    // notify WDT still working
    //feedLoopWDT();
    return 1; // File was fetched from web
  }
}

void create_players_select(){
  lv_list_screen_player(); //crea la lista de players
}

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.print("Day of week: ");
  Serial.println(&timeinfo, "%A");
  Serial.print("Month: ");
  Serial.println(&timeinfo, "%B");
  Serial.print("Day of Month: ");
  Serial.println(&timeinfo, "%d");
  Serial.print("Year: ");
  Serial.println(&timeinfo, "%Y");
  Serial.print("Hour: ");
  Serial.println(&timeinfo, "%H");
  Serial.print("Hour (12 hour format): ");
  Serial.println(&timeinfo, "%I");
  
  Serial.print("Minute: ");
  Serial.println(&timeinfo, "%M");
  Serial.print("Second: ");
  Serial.println(&timeinfo, "%S");

  Serial.println("Time variables");
  char timeHour[3];
  strftime(timeHour,3, "%H", &timeinfo);
  Serial.println(timeHour);
  char timeWeekDay[10];
  strftime(timeWeekDay,10, "%A", &timeinfo);
  Serial.println(timeWeekDay);
  Serial.println();
  //variables
  int minutes = timeinfo.tm_min;
  int hours = timeinfo.tm_hour;
  int seconds = timeinfo.tm_sec;
  lv_img_set_angle(ui_hour, hours*300);
  //lv_img_set_angle(ui_min, minutes*60);
  lv_img_set_angle(ui_sec, seconds*60);
  //sec_Animation(ui_sec, 0);
  //min_Animation(ui_min, 0);
}

void updateLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time update");
    return;
  }
  //variables
  int minutes = timeinfo.tm_min;
  int hours = timeinfo.tm_hour;
  int seconds = timeinfo.tm_sec;
  lv_label_set_text_fmt(ui_LabelScreenClock, "%02d:%02d", hours, minutes);
  lv_img_set_angle(ui_hour, (hours*300) + (minutes * 5));
  lv_img_set_angle(ui_min, minutes*60);
  lv_img_set_angle(ui_sec, seconds*60);
  //sec_Animation(ui_sec, 0);
  //min_Animation(ui_min, 0);
}

void Get_Weather(){
  if(WiFi.status()== WL_CONNECTED){
    
    Serial.println(serverPath);
    jsonBuffer = httpGETRequest(serverPath.c_str());
    //Serial.println(jsonBuffer);
    // Stream& input;
    StaticJsonDocument<2024> doc;

    DeserializationError error = deserializeJson(doc, jsonBuffer);

    if (error) {
      Serial.print("deserializeJson() weather failed: ");
      Serial.println(error.c_str());
      return;
    }
    JsonObject weather_0 = doc["weather"][0];
    //int weather_0_id = weather_0["id"]; // 800
    const char* weather_0_main = weather_0["main"]; // "Clear"
    const char* weather_0_description = weather_0["description"]; // "clear sky"
    lv_label_set_text(ui_WeatherDescription,  weather_0_description);

    const char* icon = weather_0["icon"]; // "01d"
    String icon_str =weather_0["icon"]; // "01d"
    //Serial.print("icon : ");
    //Serial.println(icon);
    icon_str[3]='/0';
    //Serial.println(icon_str);
    int icon_int = icon_str.toInt();
    //Serial.println(icon_int);
    if(icon_int == 1){
      lv_img_set_src(ui_WeatherIcon, &ui_img_clear_png);
    }
    else if(icon_int == 2) {
      lv_img_set_src(ui_WeatherIcon, &ui_img_day_partial_cloud_png);
    }
    else if(icon_int == 3 || icon_int == 4){
      lv_img_set_src(ui_WeatherIcon, &ui_img_clouds_png);
    }
    else if(icon_int == 9){
      lv_img_set_src(ui_WeatherIcon, &ui_img_drizzle_png);
    }
    else if(icon_int == 10){
      lv_img_set_src(ui_WeatherIcon, &ui_img_rain_png);
    }
    else if(icon_int == 11){
      lv_img_set_src(ui_WeatherIcon, &ui_img_thunderstorm_png);
    }
    else if(icon_int == 13){
      lv_img_set_src(ui_WeatherIcon, &ui_img_snow_png);
    }
    else if(icon_int == 50){
      lv_img_set_src(ui_WeatherIcon, &ui_img_atmosphere_png);
    }
    else {
        // Código para opción por defecto
        // Aquí puedes escribir el código que deseas ejecutar si el texto no coincide con ninguna opción
        // Por ejemplo:
        Serial.println("Opción no válida");
    }
    //const char* base = doc["base"]; // "stations"

    JsonObject main = doc["main"];
    float main_temp = main["temp"]; // 301.83
    lv_label_set_text_fmt(ui_WeatherDegrees,"%dºC" ,int(main_temp));
    //float main_feels_like = main["feels_like"]; // 300.88
    float main_temp_min = main["temp_min"]; // 301.83
    float main_temp_max = main["temp_max"]; // 302.73
    lv_label_set_text_fmt(ui_WeatherMaxMin, "Min %dºC - Max %dºC",int(main_temp_min), int(main_temp_max));
    //int main_pressure = main["pressure"]; // 1015
    //int main_humidity = main["humidity"]; // 32
    //int main_sea_level = main["sea_level"]; // 1015
    //int main_grnd_level = main["grnd_level"]; // 1008

    //int visibility = doc["visibility"]; // 10000

    //JsonObject wind = doc["wind"];
    //float wind_speed = wind["speed"]; // 2.63
    //int wind_deg = wind["deg"]; // 141
    //float wind_gust = wind["gust"]; // 2.12

    //int clouds_all = doc["clouds"]["all"]; // 0

    //long dt = doc["dt"]; // 1688319322

    JsonObject sys = doc["sys"];
    //int sys_type = sys["type"]; // 1
    //int sys_id = sys["id"]; // 6420
    //const char* sys_country = sys["country"]; // "ES"
    //long sys_sunrise = sys["sunrise"]; // 1688274307
    //long sys_sunset = sys["sunset"]; // 1688326895

    //int timezone = doc["timezone"]; // 7200
    //long id = doc["id"]; // 2514169
    const char* name = doc["name"]; // "Marbella"
    //int cod = doc["cod"]; // 200

    //lv_img_set_src(ui_WeatherIcon, &ui_img_tornado_png);
    //lv_img_set_src(ui_WeatherIcon, &ui_img_clear_png);
  } 
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
   // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}
