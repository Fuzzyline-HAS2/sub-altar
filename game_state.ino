#include "sub-altar.h"

/**
 * @brief DB gamestate가 setting 일 때 한번동작하는 코드
 */
void SettingFunc()
{
    activate_bool = false;
    sendCommand("page pgSetting");
    NeoFunc = NeoNo;
    NeopixelSet(white);
}

/**
 * @brief DB gamestate가 ready 일 때 한번동작하는 코드
 */

void ReadyFunc()
{
    activate_bool = false;
    sendCommand("page pgInfo");
    NeoFunc = NeoBeforeTagger;
}

/**
 * @brief DB gamestate가 activate 일 때 반복동작하는 코드
 */
void ActivateFunc()
{
    RfidLoop();
}

/**
 * @brief DB gamestate가 activate 일 때 한번동작하는 코드
 */
void ActivateRunOnce()
{
    activate_bool = true;
}

void DataChange()
{
    if (!(const char *)my["device_name"])
    {
        Serial.println("[DataChange] 서버 데이터 없음, 스킵");
        return;
    }

    static StaticJsonDocument<1000> cur;

    String cmd;

    bool brightness_changed = ((int)my["brightness"] != (int)cur["brightness"]);

    if ((String)(const char *)my["game_state"] != (String)(const char *)cur["game_state"])
    {
        if ((String)(const char *)my["game_state"] == "setting")
        {
            if (brightness_changed) SetBrightness((int)my["brightness"]);
            SettingFunc();
        }
        else if ((String)(const char *)my["game_state"] == "ready")
        {
            if (brightness_changed) SetBrightness((int)my["brightness"]);
            ReadyFunc();
        }
        else if ((String)(const char *)my["game_state"] == "activate")
        {
            if (brightness_changed) SetBrightness((int)my["brightness"]);
            ActivateRunOnce();
        }
    }
    else if (brightness_changed)
    {
        SetBrightness((int)my["brightness"]);
    }

    if ((String)(const char *)my["device_state"] != (String)(const char *)cur["device_state"])
    {
        if ((String)(const char *)my["device_state"] == "activate")
        {
            sendCommand("page pgChipCount");
            NeoFunc = NeoGaming;
        }
        else if ((String)(const char *)my["device_state"] == "player_win")
        {
            sendCommand("page pgTaggerLose");
            NeoFunc = NeoLose;
        }
        else if ((String)(const char *)my["device_state"] == "player_lose")
        {
            sendCommand("page pgTaggerWin");
            NeoFunc = NeoWin;
        }
        else if ((String)(const char *)my["device_state"] == "blink")
        {
            if (rfid_tag) return;
            sendCommand("page pgAltarTag");
            NeoFunc = NeoTagger;
        }
        else if ((String)(const char *)my["device_state"] == "github")
        {
            ota.check();
        }
    }

    if((int)my["taken_chip"] != (int)cur["taken_chip"])
    {
        cmd = "pgChipCount.vSacrificeChip.val=" + (String)(int)my["taken_chip"];
        sendCommand(cmd.c_str());
        // 타이머가 이미 만료된 경우(서버 응답이 2초 이후) 페이지 전환
        if (!keep_tag_timer.isEnabled(keep_tag_timer_id))
            sendCommand("page pgChipCount");
    }
    if((int)my["max_taken_chip"] != (int)cur["max_taken_chip"])
    {
        cmd = "pgChipCount.vMaxChip.val=" + (String)(int)my["max_taken_chip"];
        sendCommand(cmd.c_str());
    }
    SyncLanguage();

    Serial.println("Data Change");
    cur = my;
}