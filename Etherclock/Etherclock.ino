/*-------------------------------------------------------------------------
This source file is a part of Office Clock

For the latest info, see http://www.marrin.org/

Copyright (c) 2017, Chris Marrin
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer.
      
    - Redistributions in binary form must reproduce the above copyright 
      notice, this list of conditions and the following disclaimer in the 
      documentation and/or other materials provided with the distribution.
      
    - Neither the name of the <ORGANIZATION> nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.
      
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
-------------------------------------------------------------------------*/

// Arduino IDE Setup:
//
// Board: LOLIN(WEMOS) D1 R2 & mini

// Etherclock3 is an ESP8266 (NodeMCU) based digital clock, using the I2C display DSP7S04B.
// Time information comes from mil::LocalTimeServer
//
// The hardware has a button on top of the box, used to display the current date

// Connections to dsp7s04b board:
//
//   1 - X
//   2 - X
//   3 - X
//   4 - X
//   5 - X
//   6 - X
//   7 - Gnd
//   8 - SCL
//   9 - SDA
//  10 - 3.3v
//
// Ports
//
//      A0 - Light sensor
//
//      D0 - GPIO16 (Do Not Use)
//      D1 - SCL
//      D2 - SDA
//      D3 - N/O Button (This is the Flash switch, as long as it's high on boot you're OK)
//      D4 - On board LED
//      D5 - X
//      D6 - X
//      D7 - X
//      D8 - X

// DSP7S04B Valid characters:
//
//		 bcd   h     no  r tu - ?
//		A C EFGHIJ L NOP  S U
//
// Missing letters: kmqvwxyz

#include <mil.h>
#include <mil/Clock.h>
#include <mil/DSP7S04B.h>

// All rates in ms

static constexpr uint32_t CountdownTimerSeconds = 20 * 60;
static constexpr uint32_t ShowInfoSeconds = 5;

static constexpr const char* ConfigPortalName = "MT Etherclock";

static constexpr const char* TimeCity = "America/Los_Angeles";
static constexpr const char* WeatherCity = "93405";
static constexpr uint8_t SelectButton = D3;
static constexpr bool InvertAmbientLightLevel = false;
static constexpr uint32_t MinLightSensorLevel = 20;
static constexpr uint32_t MaxLightSensorLevel = 500;

class Etherclock : public mil::Clock
{
public:
	Etherclock()
		: mil::Clock(TimeCity, WeatherCity, 
					 InvertAmbientLightLevel, MinLightSensorLevel, MaxLightSensorLevel, 
					 SelectButton, ConfigPortalName)
	{
	}
	
	void setup()
	{
	    Wire.begin();
		Serial.begin(115200);
		delay(500);
		setBrightness(50);
		mil::Clock::setup();
  	}
	
	void loop()
	{
		mil::Clock::loop();
	}
	
private:
  enum class Info { Date, CurTemp, LowTemp, HighTemp, Done };

	void showChars(const String& string, uint8_t dps, bool colon)
	{
		if (string.length() != 4)
		{
			showChars("Err1", 0, false);
			mil::cout << "***** Invalid string display: '" << string << "'\n";
			return;
		}
		
	    _clockDisplay.clearDisplay();
	    _clockDisplay.print(const_cast<char*>(string.c_str()));
	
		// FIXME: technically, we should be able to set any dot. For now we only ever set the rightmost
		if (dps) {
			_clockDisplay.setDot(3, true);
		}
	
		if (colon) {
			_clockDisplay.setColon(true);
		}
	}

	virtual void showTime(bool force) override
	{
	    String string = "EEEE";
	    uint8_t dps = 0;
		uint32_t t = currentTime();
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
		string += String(hour);

		uint8_t minute = timeinfo->tm_min;
		if (minute < 10) {
			string += "0";
		}
		string += String(minute);
    
	    showChars(string, dps, true);
	}

	virtual void showInfo() override
	{
		_info = Info::Date;
		showInfoSequence();
		startShowDoneTimer(8000);
	}
	
	static void showInfoSequenceTick(Etherclock* self)
	{
		self->showInfoSequence();
	}

	void showInfoSequence()
	{
		if (_info == Info::Done) {
			return;
		}
		
	    String string = "EEEE";
    
		switch(_info) {
			case Info::Date: {
				uint32_t t = currentTime();
				struct tm* timeinfo = localtime(reinterpret_cast<time_t*>(&t));
				uint8_t month = timeinfo->tm_mon + 1;
				uint8_t date = timeinfo->tm_mday;

				if (month < 10) {
					string = " ";
				} else {
					string = "";
				}
				string += String(month);
				if (date < 10) {
					string += " ";
				}
				string += String(date);
				_info = Info::CurTemp;
				break;
			}
			case Info::CurTemp: {
				uint32_t temp = currentTemp();
				string = "C";
				if (temp > 0 && temp < 100) {
					string += " ";
				}
				string += temp;
				_info = Info::LowTemp;
				break;
			}
			case Info::LowTemp: {
				uint32_t temp = lowTemp();
				string = "L";
				if (temp > 0 && temp < 100) {
					string += " ";
				}
				string += temp;
				_info = Info::HighTemp;
				break;
			}
			case Info::HighTemp: {
				uint32_t temp = highTemp();
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
		_showInfoTimer.once_ms(2000, showInfoSequenceTick, this);
	}

	virtual void showString(mil::Message m) override
	{
        String s;
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




			mil::cout << "***** Displaying: '" << s << "'\n";



		showChars(s, 0, false);
		startShowDoneTimer(2000);
	}
	
	virtual void setBrightness(uint32_t b) override
	{
	    _clockDisplay.setBrightness(b);
	}

	void showCountdown()
	{
		String s;
		uint8_t minutes = _countdownSecondsRemaining / 60;
		uint8_t seconds = _countdownSecondsRemaining % 60;
		if (minutes < 10) {
			s += " ";
		}
		if (minutes == 0) {
			s += " ";
		} else {
			s += String(minutes);
		}
		
		if (seconds < 10) {
			s += (minutes == 0) ? " " : "0";
		}
		s += String(seconds);
	    showChars(s, 0, true);
	}

	DSP7S04B _clockDisplay;
	uint16_t _countdownSecondsRemaining = 0;
	uint8_t _showInfoTimeout = 0;
	
	Ticker _showInfoTimer;
	Info _info = Info::Done;
};

Etherclock etherclock;

void setup()
{
	etherclock.setup();
}

void loop()
{
	etherclock.loop();
}

