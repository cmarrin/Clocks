/*-------------------------------------------------------------------------
    This source file is a part of Office Clock
    For the latest info, see https://github.com/cmarrin/Clocks
    Copyright (c) 2021-2024, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "OfficeClock.h"

OfficeClock::OfficeClock()
    : mil::Application(LED_BUILTIN, ConfigPortalName)
    , _clockDisplay([this]() { startShowDoneTimer(DoneTimeDuration); })
    , _brightnessManager([this](uint32_t b) { setBrightness(b); }, LightSensor, 
                         InvertAmbientLightLevel, MinLightSensorLevel, MaxLightSensorLevel, NumberOfBrightnessLevels)
    , _buttonManager([this](const mil::Button& b, mil::ButtonManager::Event e) { handleButtonEvent(b, e); })
{
    _clock = std::unique_ptr<mil::Clock>(new mil::Clock(this));
}

void
OfficeClock::setup()
{
    delay(2000);
    Application::setup();

    setTitle("<center>MarrinTech Internet Connected Office Clock</center>");

    _brightnessManager.start();
    _buttonManager.addButton(mil::Button(SelectButton, SelectButton, false, mil::Button::PinMode::Pullup));

    if (_clock) {
        _clock->setup();
    }
}
	
void
OfficeClock::loop()
{
    Application::loop();
    if (_clock) {
        _clock->loop();
    }
}

void
OfficeClock::handleButtonEvent(const mil::Button& button, mil::ButtonManager::Event event)
{
    if (button.id() == SelectButton) {
        if (event == mil::ButtonManager::Event::Click) {
            sendInput(mil::Input::Click, true);
        } else if (event == mil::ButtonManager::Event::LongPress) {
            sendInput(mil::Input::LongPress, true);
        }
    }
}

void
OfficeClock::showMain(bool force)
{
    time_t currentTime = _clock->currentTime();

    bool pm = false;
    String str;

    struct tm timeinfo;
    gmtime_r(&currentTime, &timeinfo);

    uint8_t hours = timeinfo.tm_hour;
    if (hours == 0) {
        hours = 12;
    } else if (hours >= 12) {
        pm = true;
        if (hours > 12) {
            hours -= 12;
        }
    }
    str += ToString(hours).c_str();
    str += ":";
    if (timeinfo.tm_min < 10) {
        str += "0";
    }
    str += ToString(timeinfo.tm_min).c_str();

    if (str == _lastStringSent && !force) {
        return;
    }
    _lastStringSent = str;

    _clockDisplay.showString(str.c_str());
}

void
OfficeClock::showSecondary()
{
    String time = "\v";
    time += _clock->strftime("%a %b ", _clock->currentTime()).c_str();
    String day = _clock->prettyDay(_clock->currentTime()).c_str();
    time += day.c_str();
    time = time + F("  ");
    //time = time + F("  ") + _clock->weatherConditions() + F("  Cur:") + ToString(_clock->currentTemp()).c_str() + F("`  Hi:") + ToString(_clock->highTemp()).c_str() + F("`  Lo:") + ToString(_clock->lowTemp()).c_str() + F("`");

    _clockDisplay.showString(time.c_str());
}

void
OfficeClock::showString(mil::Message m)
{
    String s;
    switch (m) {
        case mil::Message::NetConfig:
            s = F("\vConfigure WiFi. Connect to the '");
            s += ConfigPortalName;
            s += F("' wifi network from your computer or mobile device, or press [select] to retry.");
            break;
        case mil::Message::Startup:
            s = F("\vOffice Clock v3.0");
            break;
        case mil::Message::Connecting:
            s = F("\aConnecting...");
            break;
        case mil::Message::NetFail:
            s = F("\vNetwork failed, press [select] to retry.");
            break;
        case mil::Message::UpdateFail:
            s = F("\vTime or weather update failed, press [select] to retry.");
            break;
        case mil::Message::AskRestart:
            s = F("\vRestart? (long press for yes)");
            break;
        case mil::Message::AskResetNetwork:
            s = F("\vReset network? (long press for yes)");
            break;
        case mil::Message::VerifyResetNetwork:
            s = F("\vAre you sure? (long press for yes)");
            break;
        default:
            s = F("\vUnknown string error");
            break;
    }

    _clockDisplay.showString(s.c_str());
}

void
OfficeClock::setBrightness(uint32_t b) {
    // Brightness needs to be 0-31 but anything more than 15 is way too bright. Adjust
    b /= 2;

    if (b > 31) {
        b = 31;
    }
    _clockDisplay.setBrightness(b);
}
