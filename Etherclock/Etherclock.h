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

#include "mil.h"
#include "Application.h"
#include "Clock.h"

#ifdef ARDUINO
#include "DSP7S04B.h"
#endif

static constexpr const char* ConfigPortalName = "MT Etherclock";
static constexpr const char* Hostname = "xxxetherclock";

static constexpr const char* ZipCode = "93405";
static constexpr uint8_t SelectButton = 0;
static constexpr uint32_t LightSensor = 0;
static constexpr bool InvertAmbientLightLevel = false;
static constexpr uint32_t MinLightSensorLevel = 20;
static constexpr uint32_t MaxLightSensorLevel = 500;

class Etherclock : public mil::Application
{
public:
	Etherclock()
		: mil::Application(LED_BUILTIN, Hostname, ConfigPortalName)
    {
        mil::BrightnessChangeCB cb = [this](uint32_t b) { _clockDisplay.setBrightness(b); };
    
        _clock = std::unique_ptr<mil::Clock>(new mil::Clock(this, ZipCode,
                                             LightSensor,
                                             InvertAmbientLightLevel,
                                             MinLightSensorLevel,
                                             MaxLightSensorLevel,
                                             SelectButton, cb));
    }
	
	virtual void setup() override
    {
		delay(500);
		_clock->setBrightness(50);
        Application::setup();
        if (_clock) {
            _clock->setup();
        }
    }   

	virtual void loop() override
    {
        Application::loop();
        if (_clock) {
            _clock->loop();
        }
    }   

private:
    enum class Info { Date, CurTemp, LowTemp, HighTemp, Done };

	virtual void showString(mil::Message m) override;
	virtual void showMain(bool force = false) override;
    virtual void showSecondary() override;
	void showInfoSequence();
	void showChars(const std::string& string, uint8_t dps, bool colon);

    mil::DSP7S04B _clockDisplay;

	Info _info = Info::Done;
	Ticker _showInfoTimer;
    
    std::unique_ptr<mil::Clock> _clock;
    
    uint8_t _lastHour = 0;
    uint8_t _lastMinute = 0;
    uint8_t _lastDps = 0;
};
