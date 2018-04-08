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
//      D1 - Select Button (active low)
//      D2 - Next Button (active low)
//      D3 - Back Button (active low) (GPIO0, as long as it's high on boot you're OK)
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
#include <m8r/StateMachine.h>
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
MakeROMString(startupMessage, "\vOffice Clock v1.0");
static constexpr uint32_t StartupScrollRate = 50;
static constexpr uint32_t DateScrollRate = 50;
static constexpr const char* ConfigPortalName = "MT Galileo Clock";
static constexpr const char* ConfigPortalPassword = "clock99";

// Time and weather related
static constexpr char* WeatherCity = "Los_Altos";
static constexpr char* WeatherState = "CA";
MakeROMString(WUKey, "5bc1eac6864b7e57");

// Buttons
static constexpr uint8_t SelectButton = D1;
static constexpr uint8_t NextButton = D2;
static constexpr uint8_t BackButton = D3;

enum class State {
	Connecting, NetConfig, NetFail, UpdateFail, 
	Startup, ShowInfo, ShowTime, Idle,
	Setup, SetTimeDate, AskResetNetwork, VerifyResetNetwork, ResetNetwork,
	SetTimeHour, SetTimeMinute, SetTimeAMPM, SetDateMonth, SetDateDay, SetDateYear
};

enum class Input { Idle, SelectClick, SelectLongPress, Next, Back, ScrollDone, Connected, NetConfig, NetFail, UpdateFail };

class OfficeClock
{
public:
	OfficeClock()
		: _stateMachine([this](const String s) { _clockDisplay.showString(s); })
		, _clockDisplay([this]() { scrollComplete(); })
		, _buttonManager([this](const m8r::Button& b, m8r::ButtonManager::Event e) { handleButtonEvent(b, e); })
		, _wUnderground(WUKey, WeatherCity, WeatherState, [this]() { _needsUpdateInfo = true; })
		, _brightnessManager([this](uint8_t b) { handleBrightnessChange(b); }, LightSensor, MaxAmbientLightLevel, NumberOfBrightnessLevels)
		, _blinker(BUILTIN_LED, BlinkSampleRate)
	{ }
	
	void setup()
	{
		Serial.begin(115200);
		delay(500);
  
		m8r::cout << "\n\n" << startupMessage << "\n\n";
      
		_clockDisplay.setBrightness(0);
    
		_buttonManager.addButton(m8r::Button(SelectButton, SelectButton));
		_buttonManager.addButton(m8r::Button(NextButton, NextButton));
		_buttonManager.addButton(m8r::Button(BackButton, BackButton));
		
		_wifiManager.setAPCallback([this](WiFiManager* wifiManager) {
			m8r::cout << L_F("Entered config mode:ip=") << WiFi.softAPIP() << L_F(", ssid='") << wifiManager->getConfigPortalSSID() << L_F("'\n");
			netConfigStarted();
		});

		startStateMachine();

		_secondTimer.attach_ms(1000, secondTick, this);
	}
	
	void loop()
	{
		if (_needsUpdateInfo) {
			_needsUpdateInfo = false;
			if (_wUnderground.update()) {
				_currentTime = _wUnderground.currentTime();
				_stateMachine.sendInput(Input::Idle);
			} else {
				// FIXME: Need to do something here
			}
		}
	}
	
	void netConfigStarted()
	{
		_blinker.setRate(ConfigRate);
		_stateMachine.sendInput(Input::NetConfig);
	}

private:
	void startNetwork()
	{
		_blinker.setRate(ConnectingRate);

		//reset settings - for testing
		//_wifiManager.resetSettings();

		if (!_wifiManager.autoConnect(ConfigPortalName, ConfigPortalPassword)) {
			m8r::cout << L_F("*** Failed to connect and hit timeout\n");
			ESP.reset();
			delay(1000);
		}

		m8r::cout << L_F("Wifi connected, IP=") << WiFi.localIP() << L_F("\n");
	
		_blinker.setRate(ConnectedRate);

		_stateMachine.sendInput(Input::Connected);
	}
	
	void startStateMachine()
	{
		_stateMachine.addState(State::Connecting, [this] {
			_clockDisplay.showString("\aConnecting...");
			startNetwork();
			_needsUpdateInfo = true;
		},
			{
				  { Input::ScrollDone, State::Connecting }
				, { Input::Connected, State::Startup }
				, { Input::NetFail, State::NetFail }
				, { Input::NetConfig, State::NetConfig }
			}
		);
		_stateMachine.addState(State::NetConfig, [this] { _clockDisplay.showString("\vWaiting for network. See back of clock or press [select]."); },
			{
				  { Input::ScrollDone, State::NetConfig }
				, { Input::SelectClick, State::Setup }
				, { Input::Connected, State::Startup }
				, { Input::NetFail, State::NetFail }
			}
		);
		_stateMachine.addState(State::NetFail, [this] { _clockDisplay.showString("\vNetwork failed, press [select]"); },
			{
				  { Input::ScrollDone, State::NetFail }
  				, { Input::SelectClick, State::Setup }
			}
		);
		_stateMachine.addState(State::UpdateFail, [this] { _clockDisplay.showString("\vTime update failed, retrying..."); },
			{
				  // { Input::ScrollDone, State::GetInfo }
			}
		);
		_stateMachine.addState(State::Startup, [this] { _clockDisplay.showString(startupMessage); },
			{
				  { Input::ScrollDone, State::ShowTime }
    			, { Input::SelectClick, State::Setup }
			}
		);
		_stateMachine.addState(State::ShowInfo, [this] { showInfo(); },
			{
				  { Input::ScrollDone, State::ShowTime }
				, { Input::SelectClick, State::ShowTime }
			}
		);
		
		_stateMachine.addState(State::ShowTime, [this] { _clockDisplay.showTime(_currentTime, true); }, State::Idle);
		
		_stateMachine.addState(State::Idle, [this] { _clockDisplay.showTime(_currentTime); },
			{
				  { Input::SelectClick, State::ShowInfo }
				, { Input::SelectLongPress, State::Setup }
				, { Input::Idle, State::Idle }
			}
		);
		_stateMachine.addState(State::Setup, "\aSetup?",
			{
  				  { Input::SelectClick, State::SetTimeDate }
  				, { Input::Next, State::ResetNetwork }
				, { Input::Back, State::ShowTime }
			}
		);
		_stateMachine.addState(State::SetTimeDate, "\aTime/date?",
			{
				  { Input::SelectClick, State::SetTimeHour }
				, { Input::Next, State::AskResetNetwork }
				, { Input::Back, State::Setup }
			}
		);
		_stateMachine.addState(State::AskResetNetwork, "\aReset?",
			{
				  { Input::SelectClick, State::VerifyResetNetwork }
				, { Input::Next, State::SetTimeDate }
				, { Input::Back, State::Setup }
			}
		);
		_stateMachine.addState(State::VerifyResetNetwork, "\aYou sure?",
			{
				  { Input::SelectClick, State::ResetNetwork }
				, { Input::Next, State::SetTimeDate }
				, { Input::Back, State::Setup }
			}
		);
		_stateMachine.addState(State::ResetNetwork, [this] { _needsNetworkReset = true; }, State::NetConfig);
		
		_stateMachine.gotoState(State::Connecting);
	}
	
	void showInfo()
	{
		String time = "\v";
		time += _wUnderground.strftime("%a %b ", _currentTime);
		String day = _wUnderground.prettyDay(_currentTime);
		day.trim();
		time += day;
		time = time + "  Forecast:" + _wUnderground.conditions() + "  Currently " + _wUnderground.currentTemp() + "`  High " + _wUnderground.highTemp() + "`  Low " + _wUnderground.lowTemp() + "`";
		_clockDisplay.showString(time);
	}
	
	void scrollComplete() { _stateMachine.sendInput(Input::ScrollDone); }

	void handleButtonEvent(const m8r::Button& button, m8r::ButtonManager::Event event)
	{
		switch(button.id()) {
			case SelectButton:
			if (event == m8r::ButtonManager::Event::Click) {
				_stateMachine.sendInput(Input::SelectClick);
				m8r::cout << "SelectClick\n";
			} else if (event == m8r::ButtonManager::Event::LongPress) {
				_stateMachine.sendInput(Input::SelectLongPress);
				m8r::cout << "SelectLongPress\n";
			}
			break;

			case NextButton:
			if (event == m8r::ButtonManager::Event::Click) {
				_stateMachine.sendInput(Input::Next);
				m8r::cout << "Next Click\n";
			}
			break;

			case BackButton:
			if (event == m8r::ButtonManager::Event::Click) {
				_stateMachine.sendInput(Input::Back);
				m8r::cout << "Back Click\n";
			}
			break;
		}
	}

	void handleBrightnessChange(uint8_t brightness)
	{
		m8r::cout << L_F("*** setting brightness to ") << brightness << L_F("\n");
		_clockDisplay.setBrightness(static_cast<float>(brightness) / (NumberOfBrightnessLevels - 1));
	}
	
	static void secondTick(OfficeClock* self)
	{
		self->_currentTime++;
		self->_stateMachine.sendInput(Input::Idle);
	}

	WiFiManager _wifiManager;
	m8r::StateMachine<State, Input> _stateMachine;
	m8r::Max7219Display _clockDisplay;
	m8r::ButtonManager _buttonManager;
	m8r::WUnderground _wUnderground;
	m8r::BrightnessManager _brightnessManager;
	m8r::Blinker _blinker;
	Ticker _secondTimer;
	uint32_t _currentTime = 0;
	bool _needsUpdateInfo = false;
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

