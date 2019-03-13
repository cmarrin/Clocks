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
// Time information comes from m8r::LocalTimeServer
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
//      D3 - N/O Button (This is the Flash switch, as long as it's high on boot you're OK
//      D4 - On board LED
//      D5 - X
//      D6 - X
//      D7 - X
//      D8 - X

// DSP7S04B Valid characters:
//
//		 bcd   h     no  r tu -
//		A C EFGHIJ L NOP  S U
//
// Missing letters: kmqvwxyz

#include <m8r.h>
#include <m8r/Blinker.h>
#include <m8r/BrightnessManager.h>
#include <m8r/ButtonManager.h>
#include "DSP7S04B.h"
#include <m8r/StateMachine.h>
#include <m8r/LocalTimeServer.h>
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
static constexpr bool InvertAmbientLightLevel = false;
static constexpr uint32_t MaxAmbientLightLevel = 800;
static constexpr uint32_t MinAmbientLightLevel = 0;
static constexpr uint32_t NumberOfBrightnessLevels = 255;
static constexpr int32_t MinBrightness = 0;
static constexpr int32_t MaxBrightness = 64;

static constexpr uint32_t CountdownTimerSeconds = 20 * 60;
static constexpr uint32_t ShowInfoSeconds = 5;

static constexpr const char* ConfigPortalName = "MT Etherclock";

// Display related
MakeROMString(startupMessage, "EC-4");

// Time related
static constexpr char* TimeCity = "America/Los_Angeles";
MakeROMString(APIKey, "OFTZYMX4MSPG");

// Buttons
static constexpr uint8_t SelectButton = D3;

enum class State {
	Connecting, ShowInfo, ShowTime, CountdownTimerAsk, CountdownTimerStart, CountdownTimerCount, CountdownTimerDone,
};

enum class Input { Connected, ShowTime, SelectClick, SelectLongPress, CountdownTimerCount, CountdownTimerDone, ShowInfoDone };

class Etherclock
{
public:
	Etherclock()
		: _stateMachine([this](const String s) { _clockDisplay.print(s.c_str()); })
		, _buttonManager([this](const m8r::Button& b, m8r::ButtonManager::Event e) { handleButtonEvent(b, e); })
		, _localTimeServer(APIKey, TimeCity, [this]() { _needsUpdateInfo = true; })
		, _brightnessManager([this](uint32_t b) { handleBrightnessChange(b); }, 
							 LightSensor, InvertAmbientLightLevel, MinAmbientLightLevel, MaxAmbientLightLevel,
							 NumberOfBrightnessLevels, MinBrightness, MaxBrightness)
		, _blinker(BUILTIN_LED, BlinkSampleRate)
	{
	}
	
	void setup()
	{
	    Wire.begin();
		Serial.begin(115200);
		delay(500);
  
		m8r::cout << "\n\nStarting " << startupMessage << "...\n\n";
		_clockDisplay.print(String(startupMessage).c_str());
	    _clockDisplay.setBrightness(50);
		
		_brightnessManager.start();
		
		_buttonManager.addButton(m8r::Button(SelectButton, SelectButton));
		startStateMachine();
		
		_secondTimer.attach_ms(1000, [this]()
		{
			_currentTime++;
			if (_showInfoTimeout > 0) {
				if (--_showInfoTimeout == 0) {
					_stateMachine.sendInput(Input::ShowInfoDone);
					return;
				}
			} else if (_countdownSecondsRemaining > 0) {
				if (--_countdownSecondsRemaining == 0) {
					_stateMachine.sendInput(Input::CountdownTimerDone);
				} else {
					showCountdown();
				}
			}
			_stateMachine.sendInput(Input::ShowTime);
		});
	}
	
	void loop()
	{
		if (_needsUpdateInfo) {
			_needsUpdateInfo = false;
			
			if (_localTimeServer.update()) {
				_currentTime = _localTimeServer.currentTime();
				_stateMachine.sendInput(Input::ShowTime);
			}
		}
	}
	
private:
	void startNetwork()
	{
		_blinker.setRate(ConnectingRate);
		
		WiFiManager wifiManager;

		//wifiManager.resetSettings();			
		
		wifiManager.setAPCallback([this](WiFiManager* wifiManager) {
			m8r::cout << L_F("Entered config mode:ip=") << WiFi.softAPIP() << L_F(", ssid='") << wifiManager->getConfigPortalSSID() << L_F("'\n");
			_blinker.setRate(ConfigRate);
		});

		if (!wifiManager.autoConnect(ConfigPortalName)) {
			m8r::cout << L_F("*** Failed to connect and hit timeout\n");
			ESP.reset();
			delay(1000);
		}
		
        WiFiMode_t currentMode = WiFi.getMode();
		m8r::cout << L_F("Wifi connected, Mode=") << wifiManager.getModeString(currentMode) << L_F(", IP=") << WiFi.localIP() << m8r::endl;
	
		_blinker.setRate(ConnectedRate);

		delay(500);
		_needsUpdateInfo = true;
	}
	
	void startStateMachine()
	{
		_stateMachine.addState(State::Connecting, [this] { startNetwork(); },
			{
				  { Input::ShowTime, State::ShowTime }
			}
		);

		_stateMachine.addState(State::ShowTime, [this] { showTime(); },
			{
			  	  { Input::SelectClick, State::ShowInfo }
			  	, { Input::SelectLongPress, State::CountdownTimerAsk }
				, { Input::ShowTime, State::ShowTime }
			}
		);
		
		_stateMachine.addState(State::ShowInfo, [this] { showInfo(); _showInfoTimeout = ShowInfoSeconds; },
			{
				  { Input::ShowInfoDone, State::ShowTime }
				, { Input::SelectClick, State::ShowTime }
			}
		);
		
		// Countdown
		_stateMachine.addState(State::CountdownTimerAsk, L_F("cnt7"),
			{
  				  { Input::SelectClick, State::ShowTime }
  				, { Input::SelectLongPress, State::CountdownTimerStart }
			}
		);
		
		_stateMachine.addState(State::CountdownTimerStart, [this] { _countdownSecondsRemaining = CountdownTimerSeconds; }, State::CountdownTimerCount);
		
		_stateMachine.addState(State::CountdownTimerCount, [this] {
			showCountdown();
		},
			{
			      { Input::CountdownTimerCount, State::CountdownTimerCount }
			    , { Input::CountdownTimerDone, State::CountdownTimerDone }
				, { Input::SelectClick, State::ShowTime }
			}
		);
		
		_stateMachine.addState(State::CountdownTimerDone, L_F("donE"),
			{
  				  { Input::SelectClick, State::ShowTime }
			}
		);

		_stateMachine.gotoState(State::Connecting);
	}
	
	void showChars(const String& string, uint8_t dps, bool colon)
	{
		static String lastStringSent;
		if (string == lastStringSent) {
			return;
		}
		lastStringSent = string;
	
		assert(string.length() == 4);
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

	void showTime()
	{
	    String string = "EEEE";
	    uint8_t dps = 0;
		struct tm* timeinfo = localtime(reinterpret_cast<time_t*>(&_currentTime));
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

	void showInfo()
	{
	    String string = "EEEE";
    
		struct tm* timeinfo = localtime(reinterpret_cast<time_t*>(&_currentTime));
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
	    showChars(string, 0, false);
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
	
	m8r::StateMachine<State, Input> _stateMachine;
	DSP7S04B _clockDisplay;
	m8r::ButtonManager _buttonManager;
	m8r::LocalTimeServer _localTimeServer;
	m8r::BrightnessManager _brightnessManager;
	m8r::Blinker _blinker;
	Ticker _secondTimer;
	uint32_t _currentTime = 0;
	bool _needsUpdateInfo = false;
	uint16_t _countdownSecondsRemaining = 0;
	uint8_t _showInfoTimeout = 0;
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

