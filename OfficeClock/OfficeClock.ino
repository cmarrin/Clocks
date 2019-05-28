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
// It uses the m8r::LocalTimeServer to get the time and m8r::WeatherServer for
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

#include <m8r.h>
#include <m8r/Blinker.h>
#include <m8r/BrightnessManager.h>
#include <m8r/ButtonManager.h>
#include <m8r/Max7219Display.h>
#include <m8r/StateMachine.h>
#include <m8r/LocalTimeServer.h>
#include <m8r/WeatherServer.h>
#include <WiFiManager.h>
#include <Ticker.h>
#include <assert.h>
#include <time.h>

// All rates in ms

// Number of ms LED stays off in each mode
static constexpr uint32_t ConnectingRate = 400;
static constexpr uint32_t ConfigRate = 100;
static constexpr uint32_t ConnectedRate = 1900;
static constexpr uint32_t BlinkSampleRate = 10;

// BrightnessManager settings
static constexpr uint32_t LightSensor = A0;
static constexpr bool InvertAmbientLightLevel = true;
static constexpr uint32_t MaxAmbientLightLevel = 900;
static constexpr uint32_t MinAmbientLightLevel = 100;
static constexpr uint32_t NumberOfBrightnessLevels = 15;

// Display related
MakeROMString(startupMessage, "\vOffice Clock v1.0");
static constexpr uint32_t StartupScrollRate = 50;
static constexpr uint32_t DateScrollRate = 50;
static constexpr const char* ConfigPortalName = "MT Galileo Clock";
static constexpr const char* ConfigPortalPassword = "";

// Time and weather related
MakeROMString(TimeAPIKey, "OFTZYMX4MSPG");
MakeROMString(TimeCity, "America/Los_Angeles");
MakeROMString(WeatherAPIKey, "1a3bb16aff43416b9c9144158191705");
MakeROMString(WeatherCity, "94022");


// Buttons
static constexpr uint8_t SelectButton = D1;
static constexpr uint8_t NextButton = D2;
static constexpr uint8_t BackButton = D3;

enum class State {
	Connecting, NetConfig, NetFail, UpdateFail, 
	Startup, ShowInfo, ShowTime, Idle,
	AskResetNetwork, VerifyResetNetwork, ResetNetwork,
	AskRestart, Restart
};

enum class Input { Idle, SelectClick, SelectLongPress, ScrollDone, Connected, NetConfig, NetFail, UpdateFail };

class OfficeClock
{
public:
	OfficeClock()
		: _stateMachine([this](const String s) { _clockDisplay.showString(s); }, { { Input::SelectLongPress, State::AskRestart } })
		, _clockDisplay([this]() { scrollComplete(); })
		, _buttonManager([this](const m8r::Button& b, m8r::ButtonManager::Event e) { handleButtonEvent(b, e); })
		, _localTimeServer(TimeAPIKey, TimeCity, [this]() { _needsUpdateTime = true; })
		, _weatherServer(WeatherAPIKey, WeatherCity, [this]() { _needsUpdateWeather = true; })
		, _brightnessManager([this](uint32_t b) { handleBrightnessChange(b); }, LightSensor, InvertAmbientLightLevel, MinAmbientLightLevel, MaxAmbientLightLevel, NumberOfBrightnessLevels)
		, _blinker(BUILTIN_LED, BlinkSampleRate)
	{
		memset(&_settingTime, 0, sizeof(_settingTime));
		_settingTime.tm_mday = 1;
		_settingTime.tm_year = 100;
	}
	
	void setup()
	{
		Serial.begin(115200);
		delay(500);
  
		m8r::cout << "\n\n" << startupMessage << "\n\n";
      
		_brightnessManager.start();

		_buttonManager.addButton(m8r::Button(SelectButton, SelectButton, true, m8r::Button::PinMode::Float));
		
		startStateMachine();

		_secondTimer.attach_ms(1000, secondTick, this);
	}
	
	void loop()
	{
		if (_needsUpdateTime) {
			_needsUpdateTime = false;
			
			if (_enableNetwork) {
				if (_localTimeServer.update()) {
					_currentTime = _localTimeServer.currentTime();
					_stateMachine.sendInput(Input::Idle);
				} else {
					_stateMachine.sendInput(Input::UpdateFail);
				}
			}
		}
		if (_needsUpdateWeather) {
			_needsUpdateWeather = false;
			
			if (_enableNetwork) {
				if (_weatherServer.update()) {
					_stateMachine.sendInput(Input::Idle);
				} else {
					_stateMachine.sendInput(Input::UpdateFail);
				}
			}
		}
		if (_needsNetworkReset) {
			startNetwork();
		}
	}
	
private:
	void startNetwork()
	{
		_blinker.setRate(ConnectingRate);
		
		WiFiManager wifiManager;

		if (_needsNetworkReset) {
			_needsNetworkReset = false;
			wifiManager.resetSettings();			
		}
		
		wifiManager.setAPCallback([this](WiFiManager* wifiManager) {
			m8r::cout << L_F("Entered config mode:ip=") << WiFi.softAPIP() << L_F(", ssid='") << wifiManager->getConfigPortalSSID() << L_F("'\n");
			_blinker.setRate(ConfigRate);
			_stateMachine.sendInput(Input::NetConfig);
			_enteredConfigMode = true;
		});

		if (!wifiManager.autoConnect(ConfigPortalName)) {
			m8r::cout << L_F("*** Failed to connect and hit timeout\n");
			ESP.reset();
			delay(1000);
		}
		
		if (_enteredConfigMode) {
			// If we've been in config mode, the network doesn't startup correctly, let's reboot
			ESP.reset();
			delay(1000);
		}

        WiFiMode_t currentMode = WiFi.getMode();
		m8r::cout << L_F("Wifi connected, Mode=") << wifiManager.getModeString(currentMode) << L_F(", IP=") << WiFi.localIP() << m8r::endl;
	
		_enableNetwork = true;
		_blinker.setRate(ConnectedRate);

		delay(500);
		_needsUpdateTime = true;
		_needsUpdateWeather = true;
		_stateMachine.sendInput(Input::Connected);
	}
	
	void showSettingTime(bool hour)
	{
		String s = _localTimeServer.strftime("\a%I:%M", _settingTime);
		if (s[1] == '0') {
			s[1] = ' ';
		}
		_clockDisplay.showString(s, hour ? 0 : 3, 2);
		
	}

	void showSettingAMPM()
	{
		bool pm = _settingTime.tm_hour >= 11;
		_clockDisplay.showString("\aAM/PM", pm ? 3 : 0, 2);
		
	}

	void showSettingDate(bool month)
	{
		String s = _localTimeServer.strftime("\a%b %e", _settingTime);
		if (month) {
			_clockDisplay.showString(s, 0, 3);
		} else {
			_clockDisplay.showString(s, 4, 2);
		}
	}

	void startStateMachine()
	{
		_stateMachine.addState(State::Connecting, L_F("\aConnecting..."), [this] { startNetwork(); },
			{
				  { Input::ScrollDone, State::Connecting }
				, { Input::Connected, State::Startup }
				, { Input::NetFail, State::NetFail }
				, { Input::NetConfig, State::NetConfig }
			}
		);
		_stateMachine.addState(State::NetConfig, [this] {
			String s = "\vConfigure WiFi. Connect to the '";
			s += ConfigPortalName;
			s += "' wifi network from your computer or mobile device, or press [select] to retry.";
		},
			{
			      { Input::ScrollDone, State::NetConfig }
			    , { Input::SelectClick, State::Connecting }
			    , { Input::Connected, State::Startup }
			    , { Input::NetFail, State::NetFail }
			}
		);
		_stateMachine.addState(State::NetFail, L_F("\vNetwork failed, press [select] to retry."),
			{
				  { Input::ScrollDone, State::NetFail }
  				, { Input::SelectClick, State::Connecting }
			}
		);
		_stateMachine.addState(State::UpdateFail, L_F("\vTime or weather update failed, press [select] to retry."),
			{
			      { Input::ScrollDone, State::UpdateFail }
				, { Input::SelectClick, State::Connecting }
			}
		);
		_stateMachine.addState(State::Startup, [this] { _clockDisplay.showString(startupMessage); },
			{
				  { Input::ScrollDone, State::ShowTime }
    			, { Input::SelectClick, State::ShowTime }
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
				, { Input::Idle, State::Idle }
			}
		);
		
		// Restart
		_stateMachine.addState(State::AskRestart, L_F("\vRestart? (long press for yes)"),
			{
				  { Input::SelectClick, State::AskResetNetwork }
				, { Input::SelectLongPress, State::Restart }
			}
		);
		_stateMachine.addState(State::Restart, [] { ESP.reset(); delay(1000); }, State::Connecting);
		
		// Network reset
		_stateMachine.addState(State::AskResetNetwork, L_F("\vReset network? (long press for yes)"),
			{
				  { Input::SelectClick, State::ShowTime }
				, { Input::SelectLongPress, State::VerifyResetNetwork }
			}
		);
		_stateMachine.addState(State::VerifyResetNetwork, L_F("\vAre you sure? (long press for yes)"),
			{
				  { Input::SelectClick, State::ShowTime }
				, { Input::SelectLongPress, State::ResetNetwork }
			}
		);
		_stateMachine.addState(State::ResetNetwork, [this] { _needsNetworkReset = true; }, State::NetConfig);
		
		// Start the state machine
		_stateMachine.gotoState(State::Connecting);
	}
	
	void showInfo()
	{
		String time = "\v";
		time += _localTimeServer.strftime("%a %b ", _currentTime);
		String day = _localTimeServer.prettyDay(_currentTime);
		day.trim();
		time += day;
		if (_enableNetwork) {
			time = time + L_F("  Weather:") + _weatherServer.conditions() +
						  L_F("  Cur:") + _weatherServer.currentTemp() +
						  L_F("`  Hi:") + _weatherServer.highTemp() +
						  L_F("`  Lo:") + _weatherServer.lowTemp() + L_F("`");
		}
		_clockDisplay.showString(time);
	}
	
	void scrollComplete() { _stateMachine.sendInput(Input::ScrollDone); }

	void handleButtonEvent(const m8r::Button& button, m8r::ButtonManager::Event event)
	{
		switch(button.id()) {
			case SelectButton:
			if (event == m8r::ButtonManager::Event::Click) {
				_stateMachine.sendInput(Input::SelectClick);
			} else if (event == m8r::ButtonManager::Event::LongPress) {
				_stateMachine.sendInput(Input::SelectLongPress);
			}
			break;
		}
	}

	void handleBrightnessChange(uint32_t brightness)
	{
		_clockDisplay.setBrightness(brightness);
		m8r::cout << "setting brightness to " << brightness << "\n";
	}
	
	static void secondTick(OfficeClock* self)
	{
		self->_currentTime++;
		self->_stateMachine.sendInput(Input::Idle);
	}

	m8r::StateMachine<State, Input> _stateMachine;
	m8r::Max7219Display _clockDisplay;
	m8r::ButtonManager _buttonManager;
	m8r::LocalTimeServer _localTimeServer;
	m8r::WeatherServer _weatherServer;
	m8r::BrightnessManager _brightnessManager;
	m8r::Blinker _blinker;
	Ticker _secondTimer;
	uint32_t _currentTime = 0;
	bool _needsUpdateTime = false;
	bool _needsUpdateWeather = false;
	bool _needsNetworkReset = false;
	bool _enteredConfigMode = false;
	
	struct tm  _settingTime;
	bool _settingTimeChanged = false;
	bool _enableNetwork = false;
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

