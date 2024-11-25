/*-------------------------------------------------------------------------
    This source file is a part of Etherclock
    For the latest info, see https://github.com/cmarrin/Clocks
    Copyright (c) 2021-2024, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "Etherclock.h"

void
Etherclock::showString(mil::Message m)
{
    std::string s;
    switch(m) {
        case mil::Message::NetConfig:
            s = "CNFG";
            break;
        case mil::Message::Startup:
            s = "EC-5";
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

    showChars(s, 0, false);
    startShowDoneTimer(2000);
}

void
Etherclock::showMain(bool force)
{
    std::string string = "EEEE";
    uint8_t dps = 0;
    time_t t = _clock->currentTime();
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
        showChars(string, dps, true);
    }
    
    _lastHour = hour;
    _lastMinute = minute;
    _lastDps = dps;
}

void
Etherclock::showSecondary()
{
    _info = Info::Date;
    showInfoSequence();
    startShowDoneTimer(8000);
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
        case Info::Date: {
            time_t t = _clock->currentTime();
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
            uint32_t temp = _clock->currentTemp();
            string = "C";
            if (temp > 0 && temp < 100) {
                string += " ";
            }
            string += std::to_string(temp);
            _info = Info::LowTemp;
            break;
        }
        case Info::LowTemp: {
            uint32_t temp = _clock->lowTemp();
            string = "L";
            if (temp > 0 && temp < 100) {
                string += " ";
            }
            string += std::to_string(temp);
            _info = Info::HighTemp;
            break;
        }
        case Info::HighTemp: {
            uint32_t temp = _clock->highTemp();
            string = "h";
            if (temp > 0 && temp < 100) {
                string += " ";
            }
            string += std::to_string(temp);
            _info = Info::Done;
            break;
        }
    }
    
    showChars(string, 0, false);
    _showInfoTimer.once_ms(2000, [this]() { showInfoSequence(); });
}

void
Etherclock::showChars(const std::string& string, uint8_t dps, bool colon)
{
    if (string.length() != 4) {
        showChars("Err1", 0, false);
        cout << F("***** Invalid string display: '") << string.c_str() << "'\n";
        return;
    }

    _clockDisplay.clearDisplay();
    _clockDisplay.setColon(colon);
    _clockDisplay.print(const_cast<char*>(string.c_str()));

    if (dps) {
        _clockDisplay.setDot(3, true);
    }
}
