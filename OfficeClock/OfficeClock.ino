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
// Since the MAX7219 is a 5v part and the ESP8266 is 3.3v, a 74HCT367 is used to adapt
// the voltage levels:
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
//			15		GND
//			16		5V
//
// Vcc and GND on the display, along with the same pins on the 74HCT367 connect to 5v and GND
// on the Wemos

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
static constexpr uint32_t NumberOfBrightnessLevels = 10;

// Display related
MakeROMString(startupMessage, "\vOffice Clock v1.0");
static constexpr uint32_t StartupScrollRate = 50;
static constexpr uint32_t DateScrollRate = 50;
static constexpr const char* ConfigPortalName = "MT Galileo Clock";
static constexpr const char* ConfigPortalPassword = "";

// Time and weather related
MakeROMString(TimeAPIKey, "OFTZYMX4MSPG");
MakeROMString(TimeCity, "America/Los_Angeles");
MakeROMString(WeatherAPIKey, "371015873839446ea41233016191303");
MakeROMString(WeatherCity, "Los%20Altos,CA");


// Buttons
static constexpr uint8_t SelectButton = D1;
static constexpr uint8_t NextButton = D2;
static constexpr uint8_t BackButton = D3;

enum class State {
	Connecting, NetConfig, NetFail, UpdateFail, 
	Startup, ShowInfo, ShowTime, Idle,
	Setup,
	AskResetNetwork, VerifyResetNetwork, ResetNetwork,
	SetTimeDate, AskSaveTime, SaveTime, 
		SetTimeHour, SetTimeHourNext,
		SetTimeMinute, SetTimeMinuteNext, 
		SetTimeAMPM, SetTimeAMPMNext,
		SetDateMonth, SetDateMonthNext,
		SetDateDay, SetDateDayNext,
		SetDateYear, SetDateYearNext,
	AskRestart, Restart
};

enum class Input { Idle, SelectClick, SelectLongPress, Next, Back, ScrollDone, Connected, NetConfig, NetFail, UpdateFail };

class OfficeClock
{
public:
	OfficeClock()
		: _stateMachine([this](const String s) { _clockDisplay.showString(s); }, { { Input::SelectLongPress, State::Setup } })
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

		_buttonManager.addButton(m8r::Button(SelectButton, SelectButton));
		_buttonManager.addButton(m8r::Button(NextButton, NextButton));
		_buttonManager.addButton(m8r::Button(BackButton, BackButton));
		
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
		_stateMachine.addState(State::NetConfig, L_F("\vWaiting for network. See back of clock or press [select]."),
			{
				  { Input::ScrollDone, State::NetConfig }
				, { Input::SelectClick, State::SetTimeDate }
				, { Input::Connected, State::Startup }
				, { Input::NetFail, State::NetFail }
			}
		);
		_stateMachine.addState(State::NetFail, L_F("\vNetwork failed, press [select]"),
			{
				  { Input::ScrollDone, State::NetFail }
  				, { Input::SelectClick, State::Connecting }
			}
		);
		_stateMachine.addState(State::UpdateFail, L_F("\vTime update failed, press [select]"),
			{
			  { Input::ScrollDone, State::UpdateFail }
				, { Input::SelectClick, State::Startup }
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
				, { Input::Idle, State::Idle }
			}
		);
		
		// Enter Setup
		_stateMachine.addState(State::Setup, L_F("\aSetup?"),
			{
  				  { Input::SelectClick, State::SetTimeDate }
  				, { Input::Next, State::SetTimeDate }
				, { Input::Back, State::ShowTime }
			}
		);
		
		// Save time settings if needed
		_stateMachine.addState(State::AskSaveTime, [this] {
			if (_settingTimeChanged) {
				_settingTimeChanged = false;
				_clockDisplay.showString("\aSave Time/Date?");
			} else {
				_stateMachine.gotoState(State::Setup);
			}
		},
			{
  				  { Input::SelectClick, State::SaveTime }
				, { Input::Back, State::Setup }
			}
		);
		_stateMachine.addState(State::SaveTime, [this] {
			_enableNetwork = false;
			_currentTime = static_cast<uint32_t>(std::mktime(&_settingTime));
		}, State::Setup);
		
		// Time/Date setting
		_stateMachine.addState(State::SetTimeDate, L_F("\aTime/date?"),
			{
				  { Input::SelectClick, State::SetTimeHour }
				, { Input::Next, State::AskResetNetwork }
				, { Input::Back, State::Setup }
			}
		);
		_stateMachine.addState(State::SetTimeHour, [this] { showSettingTime(true); },
			{
				  { Input::SelectClick, State::SetTimeMinute }
				, { Input::Next, State::SetTimeHourNext }
				, { Input::Back, State::AskSaveTime }
			}
		);
		_stateMachine.addState(State::SetTimeHourNext, [this] {
			_settingTimeChanged = true;
			_settingTime.tm_hour += 1;
			if (_settingTime.tm_hour == 12) {
				_settingTime.tm_hour = 0;
			} else if (_settingTime.tm_hour == 24) {
				_settingTime.tm_hour = 12;
			}
		}, State::SetTimeHour);
		_stateMachine.addState(State::SetTimeMinute, [this] { showSettingTime(false); },
			{
				  { Input::SelectClick, State::SetTimeAMPM }
				, { Input::Next, State::SetTimeMinuteNext }
				, { Input::Back, State::AskSaveTime }
			}
		);
		_stateMachine.addState(State::SetTimeMinuteNext, [this] {
			_settingTimeChanged = true;
			_settingTime.tm_min += 1;
			if (_settingTime.tm_min >= 60) {
				_settingTime.tm_min = 0;
			}
		}, State::SetTimeMinute);
		_stateMachine.addState(State::SetTimeAMPM, [this] { showSettingAMPM(); },
			{
				  { Input::SelectClick, State::SetDateMonth }
				, { Input::Next, State::SetTimeAMPMNext }
				, { Input::Back, State::AskSaveTime }
			}
		);
		_stateMachine.addState(State::SetTimeAMPMNext, [this] {
			_settingTimeChanged = true;
			_settingTime.tm_hour += (_settingTime.tm_hour >= 12) ? -12 : 12;
		}, State::SetTimeAMPM);
		_stateMachine.addState(State::SetDateMonth, [this] { showSettingDate(true); },
			{
				  { Input::SelectClick, State::SetDateDay }
				, { Input::Next, State::SetDateMonthNext }
				, { Input::Back, State::AskSaveTime }
			}
		);
		_stateMachine.addState(State::SetDateMonthNext, [this] {
			_settingTimeChanged = true;
			_settingTime.tm_mon += 1;
			if (_settingTime.tm_mon >= 12) {
				_settingTime.tm_mon = 0;
			}
		}, State::SetDateMonth);
		_stateMachine.addState(State::SetDateDay, [this] { showSettingDate(false); },
			{
				  { Input::SelectClick, State::SetDateYear }
				, { Input::Next, State::SetDateDayNext }
				, { Input::Back, State::AskSaveTime }
			}
		);
		_stateMachine.addState(State::SetDateDayNext, [this] {
			_settingTimeChanged = true;
			_settingTime.tm_mday += 1;
			
			// Allow 31 days in each month and fix it later
			if (_settingTime.tm_mday > 31) {
				_settingTime.tm_mday = 1;
			}
		}, State::SetDateDay);
		_stateMachine.addState(State::SetDateYear, [this] {
			_clockDisplay.showString(String("\a") + (_settingTime.tm_year + 1900), 0, 4);
		},
			{
				  { Input::SelectClick, State::SetTimeHour }
				, { Input::Next, State::SetDateYearNext }
				, { Input::Back, State::AskSaveTime }
			}
		);
		_stateMachine.addState(State::SetDateYearNext, [this] {
			_settingTimeChanged = true;
			_settingTime.tm_year += 1;
			
			// Only go up  to the year 2099
			if (_settingTime.tm_year >= 200) {
				_settingTime.tm_year = 1;
			}
		}, State::SetDateYear);
		
		// Network reset
		_stateMachine.addState(State::AskResetNetwork, L_F("\aReset network?"),
			{
				  { Input::SelectClick, State::VerifyResetNetwork }
				, { Input::Next, State::AskRestart }
				, { Input::Back, State::Setup }
			}
		);
		_stateMachine.addState(State::VerifyResetNetwork, L_F("\aYou sure?"),
			{
				  { Input::SelectClick, State::ResetNetwork }
				, { Input::Next, State::SetTimeDate }
				, { Input::Back, State::Setup }
			}
		);
		_stateMachine.addState(State::ResetNetwork, [this] { _needsNetworkReset = true; }, State::NetConfig);
		
		// Restart
		_stateMachine.addState(State::AskRestart, L_F("\aRestart?"),
			{
				  { Input::SelectClick, State::Restart }
				, { Input::Next, State::SetTimeDate }
				, { Input::Back, State::Setup }
			}
		);
		_stateMachine.addState(State::Restart, [] { ESP.reset(); delay(1000); }, State::Connecting);
		
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
						  L_F("  Currently ") + _weatherServer.currentTemp() +
						  L_F("`  High ") + _weatherServer.highTemp() +
						  L_F("`  Low ") + _weatherServer.lowTemp() + L_F("`");
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

			case NextButton:
			if (event == m8r::ButtonManager::Event::Click) {
				_stateMachine.sendInput(Input::Next);
			}
			break;

			case BackButton:
			if (event == m8r::ButtonManager::Event::Click) {
				_stateMachine.sendInput(Input::Back);
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

