#include "sub-altar.h"

//****************************************** Initialize ******************************************
void SensorInit()
{
  // Neopixel init
  pixels_top.begin();
  pixels_mid.begin();
  pixels_bot.begin();

  // 초기 전역 밝기 적용 (색상은 풀 밝기로 정의되어 있으므로 여기서 스케일)
  pixels_top.setBrightness(color_brightness);
  pixels_mid.setBrightness(color_brightness);
  pixels_bot.setBrightness(color_brightness);

  // Rfid init
  RfidInit();
}

//********************************************* Rfid *********************************************
/**
 * @brief RFID(=PN532) 세팅
 */
void RfidInit(void)
{
  nfc.begin(); // nfc 함수 시작
  if (!(nfc.getFirmwareVersion()))
  {
    Serial.println("!!!RFID 연결실패!!! - 계속 진행");
    has2wifi.Send((String)(const char *)my["device_name"], "device_state", "PN532");
    return;
  }
  nfc.SAMConfig(); // configure board to read RFID tags
  Serial.println("RFID 연결성공");
}

/**
 * @brief RFID 태그 인식
 */
void RfidLoop()
{
  if (!rfid_tag)
  {
    rfid_tag = true;
    rfid_timer_id = rfid_timer.setTimeout(1000, RfidTagTimerFunc);
  }
  else
  {
    return;
  }
  uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer to store the returned UID
  uint8_t uidLength;                     // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  uint8_t data[32];
  char user_data[5];
  byte pn532_packetbuffer11[64];
  pn532_packetbuffer11[0] = 0x00;
  if (nfc.sendCommandCheckAck(pn532_packetbuffer11, 1))
  { // rfid 통신 가능한 상태인지 확인
    if (nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A))
    {                                    // rfid에 tag 찍혔는지 확인용 //데이터 들어오면 uid정보 가져오기
      if (nfc.ntag2xx_ReadPage(7, data)) // ntag 데이터에 접근해서 불러와서 data행열에 저장
        CardChecking(data);
    }
  }
}

/**
 * @brief RFID에 태그된 NFC의 데이터에 따른 코드 동작
 *
 * @param rfidData 태그된 NFC의 데이터
 */
void CardChecking(uint8_t rfidData[32]) // 어떤 카드가 들어왔는지 확인용
{
  String tagUser = "";
  static String cur_tag_user = "";
  for (int i = 0; i < 4; i++) // GxPx 데이터만 배열에서 추출해서 string으로 저장
    tagUser += (char)rfidData[i];
  Serial.println("tag_user_data : " + tagUser);

  // 이미 사용된(used) 제단이면 칩 처리 없이 "사용 불가" 안내만 표시.
  // 서버 반영(device_state="used") 전이라도 로컬 래치(altar_used_local)로 즉시 차단한다.
  if ((String)(const char *)my["device_state"] == "used" || altar_used_local)
  {
    sendCommand("pgUsed.wNoChip.en=1");
    return;
  }

  // 1. 태그한 플레이어의 역할과 생명칩갯수, 최대생명칩갯수 등 읽어오기
  has2wifi.Receive(tagUser);

  if ((String)(const char *)tag["role"] == "tagger" && (int)tag["taken_chip"] > 0)
  {
    sendCommand("page pgKeepTag");

    // 봉헌 커밋 즉시 로컬 잠금. 서버가 device_state="used" 를 되돌려주기 전까지
    // 추가 태그가 pgKeepTag 로 넘어가 중복 봉헌되는 것을 막는다.
    altar_used_local = true;

    NeoFunc = NeoNo;
    for (int i = 0; i < NUMPIXELS_MID; i++)
    {
      if (i == 0)
      {
        pixels_bot.clear();
        pixels_top.clear();
        pixels_mid.clear();
      }
      lightColor(pixels_mid, purple, i);
      delay(50);
    }

    String main_altar = (String)(const char *)my["main_altar_device_name"];
    if (main_altar.length() > 0)
        // 출처 표식: 소제단 경유 제물은 "taken_chip_sub"로 보낸다.
        // 서버는 대제단 taken_chip을 똑같이 올리되 대제단 효과는 발동하지 않는다.
        // (대제단에 직접 바친 제물만 "taken_chip" → 효과 발동)
        has2wifi.Send(main_altar, "taken_chip_sub", "+1");
    else
        Serial.println("[WARN] main_altar_device_name 없음, taken_chip 전송 스킵");

    // 소제단은 1회용. 자신의 taken_chip 은 증가하지 않으므로(제물은 대제단으로 감)
    // taken_chip 반영을 기다리지 말고 즉시 device_state 를 "used" 로 전환한다.
    has2wifi.Send((String)(const char *)my["device_name"], "device_state", "used");

    has2wifi.Send((String)(const char *)tag["device_name"], "taken_chip", "-1");
    has2wifi.Send((String)(const char *)tag["device_name"], "exp", "+100");

    pixels_mid.clear();
    pixels_bot.clear();
    pixels_top.clear();

    // 페이지 전환(pgKeepTag -> pgUsed)은 Nextion 내부 타이머(3초)가 처리

    NeoFunc = NeoGaming;
  }
  else
  {
    // tagger가 아니거나(=다른 역할) tagger인데 바칠 칩이 없는 경우 → "사용 불가" 안내
    sendCommand("pgCardCheckin.wNoChip.en=1");
  }
}

bool RfidNsecTag(int sec)
{
  if (nsec_tag_num == 0 && !nsec_tag_bool)
  {
    nsec_tag_timer_id = nsec_tag_timer.setTimeout(5000, NsecTagTimerFailFunc);
    nsec_tag_bool = true;
  }
  else
  {
    nsec_tag_timer.restartTimer(nsec_tag_timer_id);
  }

  if (nsec_tag_num >= sec && nsec_tag_bool)
  {
    Serial.println("태그 성공");
    nsec_tag_timer.deleteTimer(nsec_tag_timer_id);
    nsec_tag_bool = false;
    nsec_tag_timer_id = nsec_tag_timer.setTimeout(2000, NsecTagTimerSuccessFunc);
    return true;
  }
  else
  {
    nsec_tag_num++;
  }
  return false;
}

//******************************************* Neopixel Helpers *******************************************
void NeopixelSet(int color[3])
{
  current_neopixel_color = color;
  uint32_t c = Adafruit_NeoPixel::Color(color[0], color[1], color[2]);
  pixels_top.fill(c); pixels_top.show();
  pixels_mid.fill(c); pixels_mid.show();
  pixels_bot.fill(c); pixels_bot.show();
  delay(10);
  pixels_top.show();
  pixels_mid.show();
  pixels_bot.show();
}

void ApplyBrightness(int raw)
{
  // raw: 0~255 전역 밝기. 색 배열은 풀 밝기(255)로 두고 setBrightness()로만 스케일.
  color_brightness = raw;
  pixels_top.setBrightness(raw);
  pixels_mid.setBrightness(raw);
  pixels_bot.setBrightness(raw);
  // 현재 켜져 있는 색을 새 밝기로 즉시 반영
  pixels_top.show();
  pixels_mid.show();
  pixels_bot.show();
}

void SetBrightness(int pct)
{
  int raw;
  if (pct <= 0 || pct > 100)
    raw = DEFAULT_BRIGHTNESS;
  else
    raw = map(pct, 1, 100, 1, 255);
  ApplyBrightness(raw);
}

void lightColor(Adafruit_NeoPixel &pixels, int color[3], int index)
{
  pixels.setPixelColor(index, color[0], color[1], color[2]);
  pixels.show();
}

//******************************************* Neopixel *******************************************
void NeoNo()
{
}
// A 상태
void NeoBeforeTagger()
{
  delay(100);
  static int breathe = 0;
  static bool breathe_direction = true;

  breathe += breathe_direction ? BREATHE_STEP : -BREATHE_STEP;
  if (breathe >= BREATHE_MAX) { breathe = BREATHE_MAX; breathe_direction = false; }
  else if (breathe <= 0)      { breathe = 0;           breathe_direction = true;  }

  pixels_mid.fill(Adafruit_NeoPixel::Color(breathe, 0, 0)); pixels_mid.show();
  pixels_bot.fill(Adafruit_NeoPixel::Color(breathe, 0, 0)); pixels_bot.show();
  pixels_top.fill(Adafruit_NeoPixel::Color(red[0], red[1], red[2])); pixels_top.show();
}

void NeoTagger()
{
  delay(100);
  static int breathe_2 = 0;
  static bool breathe_direction_2 = true;

  breathe_2 += breathe_direction_2 ? BREATHE_STEP : -BREATHE_STEP;
  if (breathe_2 >= BREATHE_MAX) { breathe_2 = BREATHE_MAX; breathe_direction_2 = false; }
  else if (breathe_2 <= 0)      { breathe_2 = 0;           breathe_direction_2 = true;  }

  pixels_bot.fill(Adafruit_NeoPixel::Color(breathe_2, breathe_2, breathe_2)); pixels_bot.show();

  pixels_mid.clear();
  NeoArrow();
}

void NeoTaggerTag()
{
  static int tag_neo = 0;

  pixels_mid.clear();
  pixels_bot.clear();

  lightColor(pixels_mid, purple, tag_neo);

  if (++tag_neo > NUMPIXELS_MID)
  {
    tag_neo = 0;

    pixels_mid.clear();
    pixels_bot.clear();
    pixels_top.clear();
  }
}

void NeoAfterTagger()
{
  static bool after_tagger_neo_bool = false;

  if (after_tagger_neo_bool)
  {
    after_tagger_neo_bool = false;
    pixels_mid.clear();
    pixels_bot.clear();
    pixels_top.clear();
  }
  else
  {
    after_tagger_neo_bool = true;
    NeopixelSet(purple);
  }
}

void NeoGaming()
{
  delay(100);
  static int breathe = 0;
  static bool breathe_direction = true;

  breathe += breathe_direction ? BREATHE_STEP : -BREATHE_STEP;
  if (breathe >= BREATHE_MAX) { breathe = BREATHE_MAX; breathe_direction = false; }
  else if (breathe <= 0)      { breathe = 0;           breathe_direction = true;  }

  pixels_mid.fill(Adafruit_NeoPixel::Color(breathe, 0, breathe)); pixels_mid.show();
  pixels_bot.fill(Adafruit_NeoPixel::Color(breathe, 0, breathe)); pixels_bot.show();
  NeoArrow();
}

// void NeoTakenChip()
// {
//   static int chip_neo = 0;

//   if(chip_neo == 0){
//     pixels_bot.clear();
//     pixels_top.clear();
//     pixels_mid.clear();
//   }

//   pixels_mid.lightColor(purple, chip_neo);

//   if(++chip_neo > NUMPIXELS_MID){
//     chip_neo = 0;

//     pixels_mid.clear();
//     pixels_bot.clear();
//     pixels_top.clear();
//   }
// }

void NeoWin()
{
  static bool win_neo_bool = false;
  static int win_neo = 255;   // 풀 밝기 파랑, 실제 밝기는 setBrightness()가 결정
  static int win_neo_delay = 1500;

  win_neo_delay = win_neo_delay - 100;

  if (win_neo_bool)
  {
    win_neo_bool = false;
    int win_color[3] = {0, 0, win_neo};
    NeopixelSet(win_color);
  }
  else
  {
    win_neo_bool = true;
    pixels_mid.clear();
    pixels_bot.clear();
    pixels_top.clear();
  }

  if (win_neo_delay <= 300)
  {
    pixels_mid.clear();
    pixels_bot.clear();
    pixels_top.clear();

    NeoFunc = NeoNo;
  }
  delay(win_neo_delay);
}

void NeoLose()
{
  static bool lose_neo_bool = false;
  static int lose_neo = 255;   // 풀 밝기 빨강, 실제 밝기는 setBrightness()가 결정
  static int lose_neo_delay = 1500;

  lose_neo_delay = lose_neo_delay - 100;

  // 깜빡임을 표현
  if (lose_neo_bool)
  {
    lose_neo_bool = false;
    int lose_color[3] = {lose_neo, 0, 0};
    NeopixelSet(lose_color);
  }
  else
  {
    lose_neo_bool = true;
    pixels_mid.clear();
    pixels_bot.clear();
    pixels_top.clear();
  }

  if (lose_neo_delay <= 300)
  {
    pixels_mid.clear();
    pixels_bot.clear();
    pixels_top.clear();

    NeoFunc = NeoNo;
  }
  delay(lose_neo_delay);
}

void NeoArrow()
{
  static int arrow_pattern = 0;

  switch (arrow_pattern)
  {
  case 0:
    pixels_top.clear();
    break;

  case 1:
    arrow_neo_line_1 = 0;
    arrow_neo_line_2 = 16;
    arrow_neo_line_3 = 0;
    break;

  case 2:
    arrow_neo_line_1 = 1;
    arrow_neo_line_2 = 24;
    arrow_neo_line_3 = 1;
    break;

  case 3:
    arrow_neo_line_1 = 3;
    arrow_neo_line_2 = 12;
    arrow_neo_line_3 = 3;
    break;

  case 4:
    arrow_neo_line_1 = 6;
    arrow_neo_line_2 = 6;
    arrow_neo_line_3 = 6;
    break;

  case 5:
    arrow_neo_line_1 = 12;
    arrow_neo_line_2 = 3;
    arrow_neo_line_3 = 12;
    break;

  case 6:
    arrow_neo_line_1 = 24;
    arrow_neo_line_2 = 1;
    arrow_neo_line_3 = 24;
    break;

  case 7:
    arrow_neo_line_1 = 16;
    arrow_neo_line_2 = 0;
    arrow_neo_line_3 = 16;
    break;

  default:
    break;
  }

  if (++arrow_pattern > 7)
  {
    arrow_pattern = 0;
  }

  NeoArrowSet(1, arrow_neo_line_1);
  // NeoArrowSet(2, arrow_neo_line_2);
  NeoArrowSet(3, arrow_neo_line_3);
  pixels_top.show();
}

void NeoArrowSet(int arrow_neo_line_num, int arrow_neo_line)
{
  int neo_num = 0;

  if (arrow_neo_line_num == 1)
  {
    neo_num = 0;
  }
  else if (arrow_neo_line_num == 2)
  {
    neo_num = 5;
  }
  else if (arrow_neo_line_num == 3)
  {
    neo_num = 10;
  }

  switch (arrow_neo_line)
  {
  case 0:
    pixels_top.setPixelColor(neo_num + 1, 0, 0, 0);
    pixels_top.setPixelColor(neo_num + 2, 0, 0, 0);
    pixels_top.setPixelColor(neo_num + 3, 0, 0, 0);
    pixels_top.setPixelColor(neo_num + 4, 0, 0, 0);
    pixels_top.setPixelColor(neo_num + 5, 0, 0, 0);
    break;
  case 1:
    pixels_top.setPixelColor(neo_num + 1, 0, 0, 0);
    pixels_top.setPixelColor(neo_num + 2, 0, 0, 0);
    pixels_top.setPixelColor(neo_num + 3, 0, 0, 0);
    pixels_top.setPixelColor(neo_num + 4, 0, 0, 0);
    pixels_top.setPixelColor(neo_num + 5, 255, 0, 255);
    break;
  case 3:
    pixels_top.setPixelColor(neo_num + 1, 0, 0, 0);
    pixels_top.setPixelColor(neo_num + 2, 0, 0, 0);
    pixels_top.setPixelColor(neo_num + 3, 0, 0, 0);
    pixels_top.setPixelColor(neo_num + 4, 255, 0, 255);
    pixels_top.setPixelColor(neo_num + 5, 255, 0, 255);
    break;
  case 6:
    pixels_top.setPixelColor(neo_num + 1, 0, 0, 0);
    pixels_top.setPixelColor(neo_num + 2, 0, 0, 0);
    pixels_top.setPixelColor(neo_num + 3, 255, 0, 255);
    pixels_top.setPixelColor(neo_num + 4, 255, 0, 255);
    pixels_top.setPixelColor(neo_num + 5, 0, 0, 0);
    break;
  case 12:
    pixels_top.setPixelColor(neo_num + 1, 0, 0, 0);
    pixels_top.setPixelColor(neo_num + 2, 255, 0, 255);
    pixels_top.setPixelColor(neo_num + 3, 255, 0, 255);
    pixels_top.setPixelColor(neo_num + 4, 0, 0, 0);
    pixels_top.setPixelColor(neo_num + 5, 0, 0, 0);
    break;
  case 24:
    pixels_top.setPixelColor(neo_num + 1, 255, 0, 255);
    pixels_top.setPixelColor(neo_num + 2, 255, 0, 255);
    pixels_top.setPixelColor(neo_num + 3, 0, 0, 0);
    pixels_top.setPixelColor(neo_num + 4, 0, 0, 0);
    pixels_top.setPixelColor(neo_num + 5, 0, 0, 0);
    break;
  case 16:
    pixels_top.setPixelColor(neo_num + 1, 255, 0, 255);
    pixels_top.setPixelColor(neo_num + 2, 0, 0, 0);
    pixels_top.setPixelColor(neo_num + 3, 0, 0, 0);
    pixels_top.setPixelColor(neo_num + 4, 0, 0, 0);
    pixels_top.setPixelColor(neo_num + 5, 0, 0, 0);
    break;
  default:
    break;
  }
}
