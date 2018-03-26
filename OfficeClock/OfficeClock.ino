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

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <JsonStreamingParser.h>
#include <JsonListener.h>
#include <ESP8266HTTPClient.h>
#include <Ticker.h>
#include <time.h>
#include <assert.h>
#include <m8r.h>
#include <m8r/Blinker.h>
#include <m8r/BrightnessManager.h>

#include "ClockDisplay.h"

const uint32_t ScrollRate = 200; // scroll rate in ms

// Number of ms LED stays off in each mode
constexpr uint32_t BlinkSampleRate = 10;
const uint32_t ConnectingRate = 400;
const uint32_t ConfigRate = 100;
const uint32_t ConnectedRate = 1900;

const uint32_t LightSensor = A0;
const uint32_t MaxAmbientLightLevel = 800;
const uint32_t NumberOfBrightnessLevels = 16;
bool needBrightnessUpdate = false;

int8_t curTemp, highTemp, lowTemp;
uint32_t currentTime = 0;

uint16_t timeCounterSecondsRemaining;

bool connecting = true;
bool needWeatherLookup = false;
bool needsUpdateDisplay = false;
bool showWelcomeMessage = true;

const char* weatherCity = "Los_Altos";
const char* weatherState = "CA";
constexpr char* WUKey = "5bc1eac6864b7e57";

enum class DisplayState { Time, Day, Date, CurrentTemp, HighTemp, LowTemp, CountdownTimerAsk, CountdownTimerCount, CountdownTimerDone };
DisplayState displayState = DisplayState::Time;

const char startupMessage[] = "Office Clock v1.0";

ClockDisplay clockDisplay;

class MyBrightnessManager : public BrightnessManager
{
public:
	MyBrightnessManager() : BrightnessManager(LightSensor, MaxAmbientLightLevel, NumberOfBrightnessLevels) { }
	
	virtual void callback(uint8_t brightness) override
	{
		m8r::cout << "*** setting brightness to " << brightness << "\n";
		clockDisplay.setBrightness(static_cast<float>(brightness) / (NumberOfBrightnessLevels - 1));
	}
};

MyBrightnessManager brightnessManager;

static void decimalByteToString(uint8_t v, char string[2], bool showLeadingZero)
{
    string[0] = (v < 10 && !showLeadingZero) ? ' ' : (v / 10 + '0');
    string[1] = (v % 10) + '0';
}

static void tempToString(char c, int8_t t, String& string)
{
    // string = String(c);
    // if (t < 0) {
    //     t = -t;
    //     string[1] = '-';
    // } else
    //     string[1] = ' ';
    //
    // decimalByteToString(t, &string[2], false);
}

static void showChars(const String& string, bool pm, bool colon)
{
	static String lastStringSent;
	if (string == lastStringSent) {
		return;
	}
	lastStringSent = string;
	
	assert(string.length() == 4);
	clockDisplay.setString(string, colon, pm);
		
    // bool blank = false;
    // bool endOfString = false;
    //
    // for (uint8_t stringIndex = 0, outputIndex = 0; outputIndex < 4; ++stringIndex, ++outputIndex, dps <<= 1) {
    //     if (string.c_str()[stringIndex] == '\0')
    //         endOfString = true;
    //
    //     if (endOfString)
    //         blank = true;
    //
    //     uint8_t glyph1, glyph2 = 0;
    //     bool hasSecondGlyph = m8r::SevenSegmentDisplay::glyphForChar(blank ? ' ' : string.c_str()[stringIndex], glyph1, glyph2);
    //
    //     shiftReg.send(glyph1 | ((dps & 0x08) ? 0x80 : 0), 8);
    //
    //     if (hasSecondGlyph && outputIndex != 3) {
    //         ++outputIndex;
    //         dps <<= 1;
    //         shiftReg.send(glyph2 | ((dps & 0x08) ? 0x80 : 0), 8);
    //     }
    // }
    // shiftReg.latch();
}

void updateDisplay()
{
	String string = "EEEE";
	bool pm = false;
	bool colon = false;
    
	switch (displayState) {
		case DisplayState::Date:
		case DisplayState::Day:
		case DisplayState::Time: {
			struct tm* timeinfo = localtime(reinterpret_cast<time_t*>(&currentTime));
            
			switch (displayState) {
				case DisplayState::Day:
				// RTCBase::dayString(t.day, string);
				break;
				case DisplayState::Date:
				// decimalByteToString(t.month, string, false);
				// decimalByteToString(t.date, &string[2], false);
				break;
				case DisplayState::Time: {
					colon = true;
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
					break;
				}
				default:
				break;
			}
			break;
		}
		case DisplayState::CurrentTemp:
		tempToString('C', curTemp, string);
		break;
		case DisplayState::HighTemp:
		tempToString('H', highTemp, string);
		break;
		case DisplayState::LowTemp:       
		tempToString('L', lowTemp, string);
		break;
		case DisplayState::CountdownTimerAsk:       
		string = "cnt?";
		break;
		case DisplayState::CountdownTimerCount:
		// decimalByteToString(m_timeCounterSecondsRemaining / 60, string, false);
		// decimalByteToString(m_timeCounterSecondsRemaining % 60, &string[2], true);
		break;
		case DisplayState::CountdownTimerDone:
		string = "done";
		break;
		default:
		break;
	}
    
	showChars(string, pm, colon);
}

m8r::Blinker blinker(BUILTIN_LED, BlinkSampleRate);

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager)
{
	Serial.println("Entered config mode");
	Serial.println(WiFi.softAPIP());
	//if you used auto generated SSID, print it
	Serial.println(myWiFiManager->getConfigPortalSSID());
	//entered config mode, make led toggle faster
	blinker.setRate(ConfigRate);
}

class MyJsonListener : public JsonListener
{
public:
	virtual ~MyJsonListener() { }
	
	virtual void key(String key) override
	{
		if (key == "local_epoch") {
			_waitingForLocalEpoch = true;
		} else if (key == "local_tz_offset") {
			_waitingForLocalTZOffset = true;
		}
	}
	
	virtual void value(String value) override
	{
		if (_waitingForLocalEpoch) {
			_waitingForLocalEpoch = false;
			_localEpoch = value.toInt();
		} else if (_waitingForLocalTZOffset) {
			_waitingForLocalTZOffset = false;
			_localTZOffset = value.toInt();
		} 
	}
	
	virtual void whitespace(char c) override { }
	virtual void startDocument() override { }
	virtual void endArray() override { }
	virtual void endObject() override { }
	virtual void endDocument() override { }
	virtual void startArray() override { }
	virtual void startObject() override { }
	
	uint32_t localEpoch() const { return _localEpoch; }
	int32_t localTZOffset() const { return _localTZOffset; }
	
private:
	bool _waitingForLocalEpoch = false;
	uint32_t _localEpoch = 0;
	bool _waitingForLocalTZOffset = false;
	int32_t _localTZOffset = 0;
};

void setCheckWeather()
{
	needWeatherLookup = true;
}

void checkWeather()
{
	bool failed = false;
	
	HTTPClient http;
	m8r::cout << "Getting weather and time feed...\n";

	String wuURL;
	wuURL += "http://api.wunderground.com/api/";
	wuURL += WUKey;
	wuURL +="/conditions/forecast/q/";
	wuURL += weatherState;
	wuURL += "/";
	wuURL += weatherCity;
	wuURL += ".json?a=";
	wuURL += millis();
	
	m8r::cout << "URL='" << wuURL << "'\n";
	
	http.begin(wuURL);
	int httpCode = http.GET();

	if (httpCode > 0) {
		m8r::cout << "    got response: " << httpCode << "\n";

		if(httpCode == HTTP_CODE_OK) {
			String payload = http.getString();
			m8r::cout << "Got payload, parsing...\n";
			JsonStreamingParser parser;
			MyJsonListener listener;
			parser.setListener(&listener);
			for (int i = 0; i < payload.length(); ++i) {
				parser.parse(payload.c_str()[i]);
			}
			
			currentTime = listener.localEpoch() + (listener.localTZOffset() * 3600 / 100);
			needsUpdateDisplay = true;
		}
	} else {
		m8r::cout << "[HTTP] GET... failed, error: " << http.errorToString(httpCode) << "(" << httpCode << ")\n";
		clockDisplay.setString("Failed");
		failed = true;
	}

	http.end();

	static Ticker ticker;
	
	// Check one minute past the hour. That way if daylight savings changes, we catch it sooner
	int32_t timeToNextCheck = failed ? 10 : ((60 * 60) - (static_cast<int32_t>(currentTime % (60 * 60))) + 60);
	ticker.once(timeToNextCheck, setCheckWeather);
	
	m8r::cout << "Time set to:" << ctime(reinterpret_cast<time_t*>(&currentTime)) << ", next setting in " << timeToNextCheck << " seconds\n";
}

Ticker _secondTimer;

void secondTick();

void setup()
{
	Serial.begin(115200);
  
	m8r::cout << "\n\nOffice Clock v1.0\n\n";
      
	clockDisplay.setBrightness(0);
	clockDisplay.setString("[\\]]^[_");
    
	//set led pin as output
	blinker.setRate(ConnectingRate);

	//WiFiManager
	//Local intialization. Once its business is done, there is no need to keep it around
	WiFiManager wifiManager;

	//reset settings - for testing
	//wifiManager.resetSettings();
    
	//set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
	wifiManager.setAPCallback(configModeCallback);
    
	//fetches ssid and pass and tries to connect
	//if it does not connect it starts an access point with the specified name
	//here  "AutoConnectAP"
	//and goes into a blocking loop awaiting configuration
	if (!wifiManager.autoConnect()) {
		Serial.println("failed to connect and hit timeout");
		//reset and try again, or maybe put it to deep sleep
		ESP.reset();
		delay(1000);
	}

	//if you get here you have connected to the WiFi
	m8r::cout << "Wifi connected, IP=" << WiFi.localIP() << "\n";
	blinker.setRate(ConnectedRate);
	
	_secondTimer.attach_ms(1000, secondTick);
}

bool scrollingWelcomeMessage = false;

void scrollDone()
{
	needWeatherLookup = true;
	scrollingWelcomeMessage = false;
}

void secondTick()
{
	currentTime++;
	needsUpdateDisplay = true;
}

void loop()
{
	if (needWeatherLookup) {
		checkWeather();
		needWeatherLookup = false;
	}

	if (showWelcomeMessage) {
		clockDisplay.scrollString(startupMessage, 50, scrollDone);
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

