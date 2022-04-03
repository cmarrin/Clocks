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

// Office Clock is an ESP8266 (Wemos D1 Mini) based digital clock. Display is a
// 32 x 8 dot matrix LED array driven by a MAX7219 over SPI. The SPI interface
// does not use MISO, so it only uses 3 pins. It also has an ambient light sensor
// on AO, and 3 switches to change functions and settings.
//
// It uses the mil::LocalTimeServer to get the time and mil::WeatherServer for
// current conditions, current, low and high temps


// Wemos Pinout
//
//      A0 - Light sensor
//
//      D1 - Select Button (active high)
//      D4 - On board LED
//      D5 - Matrix CLK
//      D7 - Matrix DIN
//      D8 - Matrix CS
//
// Light sensor
//
//                           Wemos A0
//                             |
//                             |
//		3.3v -------- 47KΩ ----|---- Sensor -------- GND
//                                   ^
//                                   |
//                             Longer Lead
//
// Button
//
//		D1 -------- Button (N/O) -------- GND
// 74HCT367
//
// 		Since the MAX7219 is a 5v part and the ESP8266 is 3.3v, a 74HCT367 is used to adapt
// 		the voltage levels:
//
//		74HCT367 pin
//
//			1		GND
//			2		Wemos D7 (DIN)
//			3		MAX7219 DIN
//			4		Wemos D5 (CLK)
//			5		MAX7219 CLK
//			6		Wemos D8 (CS)
//			7 		MAX7219 CS
//			8		GND
//			9		N/C
//			10		GND
//			11		N/C
//			12		GND
//			13		N/C
//			14		GND
//			15		GND
//			16		5V
//
// Power
//
//		Connect: Wemos 5v, 74HCT367 pin 16, Max7219 Vcc
//		Connect: Wemos 3.3v, Light sensor (through 47KΩ resistor)
//		Connect: Wemos GND, 74HCT367 (pins 1, 8, 10, 12, 14, 15), Light sensor (shorter lead), Max7219 GND, One end of Button

#include <mil.h>
#include <mil/Clock.h>
#include <mil/Max7219Display.h>

// All rates in ms

// Display related
static constexpr uint32_t StartupScrollRate = 50;
static constexpr uint32_t DateScrollRate = 50;
static constexpr const char* ConfigPortalName = "MT Galileo Clock";
static constexpr const char* ConfigPortalPassword = "";

static constexpr const char* TimeCity = "America/Los_Angeles";
static constexpr const char* WeatherCity = "93405";
static constexpr uint8_t SelectButton = D1;

class OfficeClock : public mil::Clock
{
public:
	OfficeClock()
		: mil::Clock("\vOffice Clock v1.0", "\aConnecting...", TimeCity, WeatherCity, SelectButton, ConfigPortalName)
		, _clockDisplay([this]() { startShowDoneTimer(100); })
	{
	}
	
	void setup()
	{
		Serial.begin(115200);
		delay(1000);
		Serial.print("\n\n");
		mil::Clock::setup();
	}
	
	void loop()
	{
		mil::Clock::loop();
	}
	
private:	
	virtual void showTime() override
	{
		_clockDisplay.showTime(currentTime());
	}

	virtual void showInfo() override
	{
		String time = "\v";
		time += Clock::strftime("%a %b ", currentTime());
		String day = prettyDay(currentTime());
		day.trim();
		time += day;
		time = time + F("  Weather:") + weatherConditions() +
					  F("  Cur:") + currentTemp() +
					  F("`  Hi:") + highTemp() +
					  F("`  Lo:") + lowTemp() + F("`");
		
		showString(time);
	}
	
	virtual void showString(const String& s) override
	{
		_clockDisplay.showString(s);
	}
	
	virtual void setBrightness(uint32_t b) override
	{
	    _clockDisplay.setBrightness(b);
	}
	
	mil::Max7219Display _clockDisplay;
};

OfficeClock officeClock;

void setup()
{
	officeClock.setup();
}

void loop()
{
	officeClock.loop();
}

