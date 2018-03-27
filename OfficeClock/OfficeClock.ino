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

/*
 * Office Clock is an ESP8266 based digital clock. LEDs are driven by serial
 * constant current drivers which take 2 pins to operate. It also has an
 * ambient light sensor on AO, and a single switch to change functions.
 * 
 * It uses a Weather Underground feed to get the current time and temps
 * Data is obtained with a request to weather underground, which looks like this:
 * 
 *     http://api.wunderground.com/api/5bc1eac6864b7e57/conditions/forecast/q/CA/Los_Altos.json
 *     
 * This will get a JSON stream with the current and forecast weather. Using JsonStreamingParser to pick out the local time, current temp and
 * forecast high and low temps. Format for these items looks like:
 * 
 *     local time:   { "current_observation": { "local_epoch": "<number>" } }
 *     current temp: { "current_observation": { "temp_f": "<number>" } }
 *     low temp:     { "forecast": { "simpleforecast": { "forecastday": [ { "low": { "fahrenheit": "<number>" } } ] } } }
 *     high temp:    { "forecast": { "simpleforecast": { "forecastday": [ { "high": { "fahrenheit": "<number>" } } ] } } }
 */

// Ports
//
//      A0 - Light sensor
//
//      D0 - GPIO16 (Do Not Use)
//      D1 - X
//      D2 - X
//      D3 - N/O Button (This is the Flash switch, as long as it's high on boot you're OK
//      D4 - On board LED
//      D5 - Matrix CLK
//      D6 - X
//      D7 - Matrix DIN
//      D8 - Matrix CS
//

#include <m8r.h>
#include <m8r/Blinker.h>
#include <m8r/BrightnessManager.h>
#include <m8r/WUnderground.h>
#include <WiFiManager.h>
#include <Ticker.h>
#include "ClockDisplay.h"
#include <time.h>
#include <assert.h>

// All rates in ms

// Number of ms LED stays off in each mode
constexpr uint32_t BlinkSampleRate = 10;
const uint32_t ConnectingRate = 400;
const uint32_t ConfigRate = 100;
const uint32_t ConnectedRate = 1900;

const uint32_t LightSensor = A0;
const uint32_t MaxAmbientLightLevel = 800;
const uint32_t NumberOfBrightnessLevels = 16;

const char startupMessage[] = "Office Clock v1.0";
const uint32_t ScrollRate = 50;
bool needsUpdateDisplay = false;
bool showWelcomeMessage = true;

uint32_t _currentTime = 0;

const char* weatherCity = "Los_Altos";
const char* weatherState = "CA";
constexpr char* WUKey = "5bc1eac6864b7e57";

ClockDisplay clockDisplay;

class MyBrightnessManager : public m8r::BrightnessManager
{
public:
	MyBrightnessManager() : m8r::BrightnessManager(LightSensor, MaxAmbientLightLevel, NumberOfBrightnessLevels) { }
	
	virtual void callback(uint8_t brightness) override
	{
		m8r::cout << "*** setting brightness to " << brightness << "\n";
		clockDisplay.setBrightness(static_cast<float>(brightness) / (NumberOfBrightnessLevels - 1));
	}
};

MyBrightnessManager brightnessManager;

class MyWUnderground : public m8r::WUnderground
{
public:
	MyWUnderground() : m8r::WUnderground(WUKey, weatherCity, weatherState) { }
	
	virtual void callback(bool succeeded) override
	{
		if (succeeded) {
			::_currentTime = currentTime();
			needsUpdateDisplay = true;
		} else {
			clockDisplay.setString("Failed");
		}
	}
};

MyWUnderground wUnderground;

void updateDisplay()
{
	bool pm = false;
	String string;
	
	struct tm* timeinfo = localtime(reinterpret_cast<time_t*>(&_currentTime));
            
	uint8_t hours = timeinfo->tm_hour;
	if (hours == 0) {
		hours = 12;
	} else if (hours >= 12) {
		pm = true;
		if (hours > 12) {
			hours -= 12;
		}
	}
	if (hours < 10) {
		string = " ";
	} else {
		string = "";
	}
	string += String(hours);
	if (timeinfo->tm_min < 10) {
		string += "0";
	}
	string += String(timeinfo->tm_min);

	static String lastStringSent;
	if (string == lastStringSent) {
		return;
	}
	lastStringSent = string;
	
	clockDisplay.setString(string, true, pm);
}

m8r::Blinker blinker(BUILTIN_LED, BlinkSampleRate);

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager)
{
	m8r::cout << "Entered config mode:ip=" << WiFi.softAPIP() << ", ssid='" << myWiFiManager->getConfigPortalSSID() << "'\n";
	blinker.setRate(ConfigRate);
}

Ticker _secondTimer;

void secondTick()
{
	_currentTime++;
	needsUpdateDisplay = true;
}

void setup()
{
	Serial.begin(115200);
  
	m8r::cout << "\n\nOffice Clock v1.0\n\n";
      
	clockDisplay.setBrightness(0);
	clockDisplay.setString("[\\]]^[_");
    
	blinker.setRate(ConnectingRate);

	WiFiManager wifiManager;

	//reset settings - for testing
	//wifiManager.resetSettings();
    
	wifiManager.setAPCallback(configModeCallback);
    
	if (!wifiManager.autoConnect()) {
		m8r::cout << "*** Failed to connect and hit timeout\n";
		ESP.reset();
		delay(1000);
	}

	m8r::cout << "Wifi connected, IP=" << WiFi.localIP() << "\n";

	blinker.setRate(ConnectedRate);

	_secondTimer.attach_ms(1000, secondTick);
}

bool scrollingWelcomeMessage = false;

void scrollDone()
{
	scrollingWelcomeMessage = false;
}

void loop()
{
	wUnderground.feed();
	if (showWelcomeMessage) {
		clockDisplay.scrollString(startupMessage, ScrollRate, scrollDone);
		showWelcomeMessage = false;
		scrollingWelcomeMessage = true;
	}
	
	if (scrollingWelcomeMessage) {
		return;
	}
	
	if (needsUpdateDisplay) {
		updateDisplay();
		needsUpdateDisplay = false;
	}
}

