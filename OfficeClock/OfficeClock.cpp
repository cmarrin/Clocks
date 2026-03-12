/*-------------------------------------------------------------------------
    This source file is a part of Office Clock
    For the latest info, see https://github.com/cmarrin/Clocks
    Copyright (c) 2021-2024, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "OfficeClock.h"

OfficeClock::OfficeClock(mil::WiFiPortal* portal, std::function<void(const uint8_t* buffer)> renderCB)
    : mil::Application(portal, ConfigPortalName, true)
    , _clockDisplay([this]() { startShowDoneTimer(DoneTimeDuration); }, renderCB)
    , _brightnessManager([this](uint32_t b) { setBrightness(b); }, LightSensor, 
                         InvertAmbientLightLevel, MinLightSensorLevel, MaxLightSensorLevel, NumberOfBrightnessLevels)
    , _buttonManager([this](const mil::Button& b, mil::ButtonManager::Event e) { handleButtonEvent(b, e); })
{
    _clock = std::unique_ptr<mil::Clock>(new mil::Clock(this));
}

void
OfficeClock::setup()
{
    delay(500);
    Application::setup();

    setTitle((std::string("<center>MarrinTech Internet Connected Office Clock v") + Version + "</center>").c_str());
    printf("Internet Connected Office Clock v%s\n", Version);

    _brightnessManager.start();
    _buttonManager.addButton(mil::Button(SelectButton, SelectButton, false, System::GPIOPinMode::InputWithPullup));
}
	
void
OfficeClock::loop()
{
    Application::loop();
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

    std::string str;

    struct tm timeinfo;
    gmtime_r(&currentTime, &timeinfo);

    uint8_t hours = timeinfo.tm_hour;
    if (hours == 0) {
        hours = 12;
    } else if (hours >= 12) {
        if (hours > 12) {
            hours -= 12;
        }
    }
    str += std::to_string(hours).c_str();
    str += ":";
    if (timeinfo.tm_min < 10) {
        str += "0";
    }
    str += std::to_string(timeinfo.tm_min).c_str();

    if (str == _lastStringSent && !force) {
        return;
    }
    _lastStringSent = str;

    _clockDisplay.showString(str.c_str());
}

void
OfficeClock::showSecondary()
{
    std::string time = "\v";
    time += _clock->strftime("%a %b ", _clock->currentTime()).c_str();
    std::string day = _clock->prettyDay(_clock->currentTime()).c_str();
    time += day.c_str();
    time = time + "  " + _clock->weatherConditions() + "  Cur:" + std::to_string(_clock->currentTemp()).c_str();
    time = time + "`  Hi:" + std::to_string(_clock->highTemp()).c_str() + "`  Lo:" + std::to_string(_clock->lowTemp()).c_str() + "`";

    _clockDisplay.showString(time.c_str());
}

void
OfficeClock::showString(mil::Message m)
{
    std::string s;
    switch (m) {
        case mil::Message::NetConfig:
            s = "\vConfigure WiFi. Connect to the '";
            s += ConfigPortalName;
            s += "' wifi network from your computer or mobile device, or press [select] to retry.";
            break;
        case mil::Message::Startup:
            s = std::string("\vOffice Clock v") + Version;
            break;
        case mil::Message::Connecting:
            s = "\aConnecting...";
            break;
        case mil::Message::NetFail:
            s = "\vNetwork failed, press [select] to retry.";
            break;
        case mil::Message::UpdateFail:
            s = "\vTime or weather update failed, press [select] to retry.";
            break;
        case mil::Message::AskRestart:
            s = "\vRestart? (long press for yes)";
            break;
        case mil::Message::AskResetNetwork:
            s = "\vReset network? (long press for yes)";
            break;
        case mil::Message::VerifyResetNetwork:
            s = "\vAre you sure? (long press for yes)";
            break;
        default:
            s = "\vUnknown string error";
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
