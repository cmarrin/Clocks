/*-------------------------------------------------------------------------
    This source file is a part of Etherclock
    For the latest info, see https://github.com/cmarrin/Clocks
    Copyright (c) 2021-2024, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

// Etherclock3 is an ESP based digital clock, using the I2C display DSP7S04B.
// Time information comes from mil::TimeWeatherServer
//
// The hardware has a button on top of the box, used to display the current date

// DSP7S04B Valid characters:
//
//		 bcd   h     no  r tu - ?
//		A C EFGHIJ L NOP  S U
//
// Missing letters: kmqvwxyz

// This is the Mac simulator. It displays the time using tigr.

#include "mil.h"
#include "Application.h"
#include "Clock.h"

static constexpr const char* ConfigPortalName = "MT Etherclock";

static constexpr const char* ZipCode = "93405";
static constexpr bool InvertAmbientLightLevel = false;
static constexpr uint32_t MinLightSensorLevel = 20;
static constexpr uint32_t MaxLightSensorLevel = 500;

class Etherclock : public mil::Application
{
public:
	Etherclock()
		: mil::Application(0, ConfigPortalName)
    {
        mil::BrightnessChangeCB cb = [this](uint32_t brightness) { setBrightness(brightness); };
    
        _clock = std::unique_ptr<mil::Clock>(new mil::Clock(this, ZipCode,
                                             InvertAmbientLightLevel,
                                             MinLightSensorLevel,
                                             MaxLightSensorLevel,
                                             0, cb));
    }
	
	virtual void loop() override
    {
        Application::loop();
        if (_clock) {
            _clock->loop();
        }
    }   

	void setBrightness(uint32_t b)
	{
	    cout << "*** Brightness set to " << b << "\n";
	}

private:
    enum class Info { Date, CurTemp, LowTemp, HighTemp, Done };

	void showChars(const std::string& string, uint8_t dps, bool colon)
	{
		if (string.length() != 4)
		{
			showChars("Err1", 0, false);
			cout << "***** Invalid string display: '" << string << "'\n";
			return;
		}
  
        cout << string.c_str() << "\n";
		
		// FIXME: technically, we should be able to set any dot. For now we only ever set the rightmost
		if (dps) {
            cout << "Dot set\n";
		}
	
		if (colon) {
            cout << "Colon set\n";
		}
	}

	virtual void showString(mil::Message m) override
	{
        std::string s;
        switch(m) {
            case mil::Message::NetConfig:
                s = F("CNFG");
                break;
            case mil::Message::Startup:
                s = F("EC-4");
                break;
            case mil::Message::Connecting:
                s = F("Conn");
                break;
            case mil::Message::NetFail:
                s = F("NtFL");
                break;
            case mil::Message::UpdateFail:
                s = F("UPFL");
                break;
            case mil::Message::AskRestart:
                s = F("Str?");
                break;
            case mil::Message::AskResetNetwork:
                s = F("rSt?");
                break;
            case mil::Message::VerifyResetNetwork:
                s = F("Sur?");
                break;
            default:
                s = F("Err ");
                break;
        }

        cout << "***** Displaying: '" << s << "'\n";

		showChars(s, 0, false);
		startShowDoneTimer(2000);
	}

	virtual void showMain(bool force = false) override
    {
	    std::string string = "EEEE";
	    uint8_t dps = 0;
		uint32_t t = _clock->currentTime();
		struct tm* timeinfo = localtime(reinterpret_cast<time_t*>(&t));
		uint8_t hour = timeinfo->tm_hour;
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
		string += hour;

		uint8_t minute = timeinfo->tm_min;
		if (minute < 10) {
			string += "0";
		}
		string += minute;
    
	    showChars(string, dps, true);
    }
	
    virtual void showSecondary() override
    {
		_info = Info::Date;
		showInfoSequence();
		startShowDoneTimer(8000);
    }

	void showInfoSequence()
	{
		if (_info == Info::Done) {
			return;
		}
		
	    std::string string = "EEEE";
    
		switch(_info) {
            case Info::Done: break;
			case Info::Date: {
				uint32_t t = _clock->currentTime();
				struct tm* timeinfo = localtime(reinterpret_cast<time_t*>(&t));
				uint8_t month = timeinfo->tm_mon + 1;
				uint8_t date = timeinfo->tm_mday;

				if (month < 10) {
					string = " ";
				} else {
					string = "";
				}
				string += month;
				if (date < 10) {
					string += " ";
				}
				string += date;
				_info = Info::CurTemp;
				break;
			}
			case Info::CurTemp: {
				uint32_t temp = _clock->currentTemp();
				string = "C";
				if (temp > 0 && temp < 100) {
					string += " ";
				}
				string += temp;
				_info = Info::LowTemp;
				break;
			}
			case Info::LowTemp: {
				uint32_t temp = _clock->lowTemp();
				string = "L";
				if (temp > 0 && temp < 100) {
					string += " ";
				}
				string += temp;
				_info = Info::HighTemp;
				break;
			}
			case Info::HighTemp: {
				uint32_t temp = _clock->highTemp();
				string = "h";
				if (temp > 0 && temp < 100) {
					string += " ";
				}
				string += temp;
				_info = Info::Done;
				break;
			}
		}
		
	    showChars(string, 0, false);
		_showInfoTimer.once_ms(2000, [this]() { showInfoSequence(); });
	}

	Info _info = Info::Done;
	Ticker _showInfoTimer;
    
    std::unique_ptr<mil::Clock> _clock;
};

int main(int argc, const char * argv[])
{
    Etherclock etherclock;
    
    etherclock.setup();
    
    while (true) {
        etherclock.loop();
    }
    return 0;
}
