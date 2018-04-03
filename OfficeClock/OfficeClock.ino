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
#include <m8r/ButtonManager.h>
#include <m8r/Max7219Display.h>
#include <m8r/MenuSystem.h>
#include <m8r/WUnderground.h>
#include <WiFiManager.h>
#include <Ticker.h>
#include <assert.h>

// All rates in ms

// Number of ms LED stays off in each mode
static constexpr uint32_t ConnectingRate = 400;
static constexpr uint32_t ConfigRate = 100;
static constexpr uint32_t ConnectedRate = 1900;
static constexpr uint32_t BlinkSampleRate = 10;

// BrightnessManager settings
static constexpr uint32_t LightSensor = A0;
static constexpr uint32_t MaxAmbientLightLevel = 800;
static constexpr uint32_t NumberOfBrightnessLevels = 16;

// Display related
MakeROMString(startupMessage, "Office Clock v1.0");
static constexpr uint32_t StartupScrollRate = 50;
static constexpr uint32_t DateScrollRate = 70;

// Time and weather related
static constexpr char* WeatherCity = "Los_Altos";
static constexpr char* WeatherState = "CA";
MakeROMString(WUKey, "5bc1eac6864b7e57");

// Buttons
static constexpr uint8_t SelectPin = D1;
static constexpr uint32_t SelectButtonId = 1;

class OfficeClock : m8r::MenuSystem
{
public:
	OfficeClock()
		: _clockDisplay(this)
		, _buttonManager([this](const m8r::Button& b, m8r::ButtonManager::Event e) { handleButtonEvent(b, e); })
		, _wUnderground(WUKey, WeatherCity, WeatherState, [this](bool succeeded) { handleWeatherInfo(succeeded); })
		, _brightnessManager([this](uint8_t b) { handleBrightnessChange(b); }, LightSensor, MaxAmbientLightLevel, NumberOfBrightnessLevels)
		, _blinker(BUILTIN_LED, BlinkSampleRate)
	{ }
	
	void setup()
	{
		Serial.begin(115200);
		delay(500);
  
		m8r::cout << "\n\n" << startupMessage << "\n\n";
      
		_clockDisplay.setBrightness(0);
		_clockDisplay.setString("Connect", m8r::Max7219Display::Font::Compact);
    
		_blinker.setRate(ConnectingRate);

		MyWiFiManager wifiManager(this);

		//reset settings - for testing
		//wifiManager.resetSettings();
    
		wifiManager.setAPCallback(reinterpret_cast<void(*)(WiFiManager*)>(configModeCallback));
    
		if (!wifiManager.autoConnect()) {
			m8r::cout << L_F("*** Failed to connect and hit timeout\n");
			ESP.reset();
			delay(1000);
		}

		m8r::cout << L_F("Wifi connected, IP=") << WiFi.localIP() << L_F("\n");
		
		_buttonManager.addButton(m8r::Button(SelectPin, SelectButtonId));
		
		std::shared_ptr<m8r::MenuItem> menu = std::make_shared<m8r::Menu>("Setup?");
		setMenu(menu);

		_blinker.setRate(ConnectedRate);

		_secondTimer.attach_ms(1000, secondTick, this);
	}
	
	void loop()
	{
		_wUnderground.feed();
		if (_showWelcomeMessage) {
			_clockDisplay.scrollString(startupMessage, StartupScrollRate);
			_showWelcomeMessage = false;
			_scrollingWelcomeMessage = true;
		}
	
		if (_scrollingWelcomeMessage) {
			return;
		}
	
		if (_needsUpdateDisplay) {
			_clockDisplay.setTime(_currentTime);
			_needsUpdateDisplay = false;
		}
	}
	
	void scrollingFinished() { _scrollingWelcomeMessage = false; }
	void setBlinkRate(uint32_t rate) { _blinker.setRate(rate); }
	void setCurrentTime(uint32_t time)
	{
		_currentTime = time;
		_needsUpdateDisplay = true;
	}
	void setFailure(const String& message)
	{
		_clockDisplay.setString(message);
	}

private:
	void handleButtonEvent(const m8r::Button& button, m8r::ButtonManager::Event event)
	{
		switch(button.id()) {
			case SelectButtonId:
			if (event == m8r::ButtonManager::Event::Click) {
				String time = _wUnderground.strftime("%a %b ", _currentTime);
				String day = _wUnderground.prettyDay(_currentTime);
				day.trim();
				time += day;
				time = time + "    Today's weather: " + _wUnderground.conditions() + "   currently " + _wUnderground.currentTemp() + "`  hi:" + _wUnderground.highTemp() + "`  lo:" + _wUnderground.lowTemp() + "`";
				_clockDisplay.scrollString(time, DateScrollRate);
			} else if (event == m8r::ButtonManager::Event::LongPress) {
				MenuSystem::start();
			}
			break;
		}
	}

	void handleWeatherInfo(bool succeeded)
	{
		if (succeeded) {
			_currentTime = _wUnderground.currentTime();
			_needsUpdateDisplay = true;
		} else {
			_clockDisplay.setString("Failed");
		}
	}

	void handleBrightnessChange(uint8_t brightness)
	{
		m8r::cout << L_F("*** setting brightness to ") << brightness << L_F("\n");
		_clockDisplay.setBrightness(static_cast<float>(brightness) / (NumberOfBrightnessLevels - 1));
	}
	
	// From MenuSystem
	virtual void showMenuItem(const m8r::MenuItem* menuItem) override
	{
		_clockDisplay.setString(menuItem->string(), m8r::Max7219Display::Font::Compact);
	}

	class MyWiFiManager : public WiFiManager
	{
	public:
		MyWiFiManager(OfficeClock* clock) : _clock(clock) { }
		OfficeClock* _clock;
	};

	class MyClockDisplay : public m8r::Max7219Display
	{
	public:
		MyClockDisplay(OfficeClock* clock) : _clock(clock) { }
		virtual void scrollDone() { _clock->scrollingFinished(); }
	
	private:
		OfficeClock* _clock;
	};
	
	static void configModeCallback (MyWiFiManager *myWiFiManager)
	{
		m8r::cout << L_F("Entered config mode:ip=") << WiFi.softAPIP() << L_F(", ssid='") << myWiFiManager->getConfigPortalSSID() << L_F("'\n");
		myWiFiManager->_clock->setBlinkRate(ConfigRate);
	}

	static void secondTick(OfficeClock* self)
	{
		self->_currentTime++;
		self->_needsUpdateDisplay = true;
	}

	MyClockDisplay _clockDisplay;
	m8r::ButtonManager _buttonManager;
	m8r::WUnderground _wUnderground;
	m8r::BrightnessManager _brightnessManager;
	m8r::Blinker _blinker;
	Ticker _secondTimer;
	uint32_t _currentTime = 0;
	bool _needsUpdateDisplay = false;
	bool _showWelcomeMessage = true;
	bool _scrollingWelcomeMessage = false;
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

