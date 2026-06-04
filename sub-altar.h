#ifndef _UPDATED_TEMPLE_H_
#define _UPDATED_TEMPLE_H_

#include "library_and_pin.h"

//============================ Global Variable ============================
void NeoNo();
void (*NeoFunc)() = NeoNo;

//============================ Hardware Serial ============================
// HardwareSerial MySerial1(1); // 사용X
HardwareSerial MySerial2(2); // Display

//================================ Wifi ==================================
HAS2_Wifi has2wifi("http://172.30.1.43");

SecureOTA ota(
  "https://raw.githubusercontent.com/Fuzzyline-HAS2/sub-altar/main/update.bin",
  "https://raw.githubusercontent.com/Fuzzyline-HAS2/sub-altar/main/version.txt",
  "https://raw.githubusercontent.com/Fuzzyline-HAS2/sub-altar/main/update.sig",
  HMAC_SECRET,
  FIRMWARE_VER
);

bool activate_bool;

void SettingFunc();
void ReadyFunc();
void ActionFunc();
void DataChange();

//=============================== Display ================================
int nextion_language = 1;  // 0=EN, 1=KO (기본값 KO)
void DisplayCheck();
void NextionReceived(String *nextion_string);
int NextionGetNum(const char *cmd);
void NextionInit();
void SyncLanguage();

//* =============================== Sensor =============================== *
/**
 * @brief Temple에 사용되는 센서, 모듈 세팅
 */
void SensorInit();

//================================ RFID ==================================
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

bool rfid_tag = false;
byte rfid_tag_count = 0; // 몇번 태그 됐는지 (= 덕트를 몇 번 사용했는지) 확인하는 변수

bool send_nfc_err = false;

void RfidInit(void);
void RfidLoop(void);
void CardChecking(uint8_t rfidData[32]);

//=============================== Neopixel ===============================
#define NUMPIXELS_TOP 24
#define NUMPIXELS_MID 16
#define NUMPIXELS_BOT 8
Adafruit_NeoPixel pixels_top(NUMPIXELS_TOP, NEOPIXEL_TOP_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels_mid(NUMPIXELS_MID, NEOPIXEL_MID_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels_bot(NUMPIXELS_BOT, NEOPIXEL_BOT_PIN, NEO_GRB + NEO_KHZ800);

int arrow_neo_line_1;
int arrow_neo_line_2;
int arrow_neo_line_3;

// Neopixel 색상정보
#define DEFAULT_BRIGHTNESS 20
int color_brightness = DEFAULT_BRIGHTNESS;

int white[3]  = {color_brightness, color_brightness, color_brightness};
int red[3]    = {color_brightness, 0,                0               };
int yellow[3] = {color_brightness, color_brightness, 0               };
int green[3]  = {0,                color_brightness, 0               };
int purple[3] = {color_brightness, 0,                color_brightness};
int blue[3]   = {0,                0,                color_brightness};

int* current_neopixel_color = white;

void NeopixelSet(int color[3]);
void ApplyBrightness(int raw);
void SetBrightness(int pct);
void lightColor(Adafruit_NeoPixel &pixels, int color[3], int index);

void NeoBeforeTagger();
void NeoTagger();
void NeoTaggerTag();
void NeoAfterTagger();
void NeoGaming();
void NeoTakenChip();
void NeoWin();
void NeoLose();
void NeoArrow();
void NeoArrowSet(int arrow_neo_line_num, int arrow_neo_line);

//================================ Timer =================================
// 1초마다 RFID 가 인식되게 타이머 설정
SimpleTimer rfid_timer;
SimpleTimer nsec_tag_timer;
SimpleTimer wifi_timer;
SimpleTimer keep_tag_timer;

int rfid_timer_id;
int nsec_tag_timer_id;
int wifi_timer_id;
int keep_tag_timer_id = -1;

int nsec_tag_num;
bool nsec_tag_bool;

void TimerInit();
void TimerRun();
void RfidTimerAssess();
void RfidTagTimerFunc();
void WifiTimerFunc();
void NsecTagTimerFailFunc();
void NsecTagTimerSuccessFunc();
void KeepTagTimerFunc();

#endif