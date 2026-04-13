/*-------------------------------------------------------------------------
    This source file is a part of Etherclock
    For the latest info, see https://github.com/cmarrin/Clocks
    Copyright (c) 2021-2024, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Etherclock.h"

static const char* TAG = "Etherclock";

Etherclock::Etherclock(mil::WiFiPortal* portal, bool buttonActiveHigh, mil::RenderCB renderCB)
    : mil::Application(portal, ConfigPortalName, true)
    , _clockDisplay(renderCB)
    , _brightnessManager([this](uint32_t b) { setBrightness(b); }, LightSensor, 
                         InvertAmbientLightLevel, MinLightSensorLevel, MaxLightSensorLevel, NumberOfBrightnessLevels)
    , _buttonManager([this](const mil::Button& b, mil::ButtonManager::Event e) { handleButtonEvent(b, e); })
    , _buttonActiveHigh(buttonActiveHigh)
{
}

void
Etherclock::setup()
{
    mil::System::delay(500);
    Application::setup();

    setTitle((std::string("<center>MarrinTech Internet Connected Office Clock v") + Version + "</center>").c_str());
    mil::System::logI(TAG, "Internet Connected Office Clock v%s\n", Version);

    _brightnessManager.start();
    _buttonManager.addButton(mil::Button(SelectButton, SelectButton, _buttonActiveHigh, mil::System::GPIOPinMode::InputWithPullup));
    setBrightness(50);
}   

void
Etherclock::loop()
{
    Application::loop();
}   

void
Etherclock::handleButtonEvent(const mil::Button& button, mil::ButtonManager::Event event)
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
Etherclock::showString(mil::Message m)
{
    std::string s;
    switch(m) {
        case mil::Message::NetConfig:
            s = "CNFG";
            break;
        case mil::Message::Startup:
            s = "EC-" + std::string(1, Version[0]);
            break;
        case mil::Message::Connecting:
            s = "Conn";
            break;
        case mil::Message::NetFail:
            s = "NtFL";
            break;
        case mil::Message::UpdateFail:
            s = "UPFL";
            break;
        case mil::Message::AskRestart:
            s = "Str?";
            break;
        case mil::Message::AskResetNetwork:
            s = "rSt?";
            break;
        case mil::Message::VerifyResetNetwork:
            s = "Sur?";
            break;
        default:
            s = "Err ";
            break;
    }

    showChars(s.c_str(), 0, false);
    startShowDoneTimer(2000);
}

void
Etherclock::showMain(bool force)
{
    std::string string = "EEEE";
    uint8_t dps = 0;
    time_t t = clock() ? clock()->currentTime() : 0;
	struct tm timeinfo;
    gmtime_r(&t, &timeinfo);
    uint8_t hour = timeinfo.tm_hour;

    if (hour == 0) {
        hour = 12;
    } else if (hour >= 12) {
        dps = 0x08;
        if (hour > 12) {
            hour -= 12;
        }
    }
    if (hour < 10) {
        string = " ";
    } else {
        string = "";
    }
    string += std::to_string(hour);

    uint8_t minute = timeinfo.tm_min;
    if (minute < 10) {
        string += "0";
    }
    string += std::to_string(minute);

    // If we are forced or the time has changed, show it
    if (force || _lastHour != hour || _lastMinute != minute || _lastDps != dps) {
        showChars(string.c_str(), dps, true);
    }
    
    _lastHour = hour;
    _lastMinute = minute;
    _lastDps = dps;
}

void
Etherclock::showSecondary()
{
    _info = Info::Day;
    showInfoSequence();
    startShowDoneTimer(SecondaryTimePerInfo * int(Info::Done));
}

void
Etherclock::showInfoSequence()
{
    if (_info == Info::Done) {
        return;
    }
    
    std::string string = "EEEE";

    switch(_info) {
        case Info::Done: break;
        case Info::Day: {
            string = clock()->strftime("%a", clock() ? clock()->currentTime() : 0);
            
            // 'M' and 'W' are the problematic character. Represent
            // 'M' with 'R7' and 'W' with 'LJ'
//            if (string[0] == 'M' || string[0] == 'm') {
//                string = "R7" + std::string(&(string.data()[1]), 2);
//            } else if (string[0] == 'W' || string[0] == 'w') {
//                string = "LJ" + std::string(&(string.data()[1]), 2);
//            }
            string += " ";
            _info = Info::Date;
            break;
        }
        case Info::Date: {
            time_t t = clock() ? clock()->currentTime() : 0;
            struct tm timeinfo;
            gmtime_r(&t, &timeinfo);
            uint8_t month = timeinfo.tm_mon + 1;
            uint8_t date = timeinfo.tm_mday;

            if (month < 10) {
                string = " ";
            } else {
                string = "";
            }
            string += std::to_string(month);
            if (date < 10) {
                string += " ";
            }
            string += std::to_string(date);
            _info = Info::CurTemp;
            break;
        }
        case Info::CurTemp: {
            uint32_t temp = clock() ? clock()->currentTemp() : 0;
            string = "C";
            if (temp > 0 && temp < 100) {
                string += " ";
            }
            string += std::to_string(temp);
            _info = Info::LowTemp;
            break;
        }
        case Info::LowTemp: {
            uint32_t temp = clock() ? clock()->lowTemp() : 0;
            string = "L";
            if (temp > 0 && temp < 100) {
                string += " ";
            }
            string += std::to_string(temp);
            _info = Info::HighTemp;
            break;
        }
        case Info::HighTemp: {
            uint32_t temp = clock() ? clock()->highTemp() : 0;
            string = "h";
            if (temp > 0 && temp < 100) {
                string += " ";
            }
            string += std::to_string(temp);
            _info = Info::Done;
            break;
        }
    }
    
    showChars(string.c_str(), 0, false);
    _showInfoTimer.once_ms(SecondaryTimePerInfo, [this]() { showInfoSequence(); });
}

void
Etherclock::showChars(const char* string, uint8_t dps, bool colon)
{
    if (strlen(string) != 4) {
        showChars("Err1", 0, false);
        mil::System::logE("invalid string display: '%s'", string);
        return;
    }

    _clockDisplay.clearDisplay();
    _clockDisplay.print(string);
    _clockDisplay.setColon(colon);

    if (dps) {
        _clockDisplay.setDot(3, true);
    }
    _clockDisplay.refresh();
}
