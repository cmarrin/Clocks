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

static constexpr const char* ConfigPortalName = "MT Etherclock";

static constexpr const char* ZipCode = "93405";
static constexpr bool InvertAmbientLightLevel = false;
static constexpr uint32_t MinLightSensorLevel = 20;
static constexpr uint32_t MaxLightSensorLevel = 500;

class Etherclock : public mil::Application
{
public:
	Etherclock()
		: mil::Application(0, ConfigPortalName)
    {
        mil::BrightnessChangeCB cb = [this](uint32_t brightness) { setBrightness(brightness); };
    
        _clock = std::unique_ptr<mil::Clock>(new mil::Clock(this, ZipCode,
                                             InvertAmbientLightLevel,
                                             MinLightSensorLevel,
                                             MaxLightSensorLevel,
                                             0, cb));
    }
	
	virtual void setup() override
    {
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

	void setBrightness(uint32_t b)
	{
	    cout << "*** Brightness set to " << b << "\n";
	}

private:
    enum class Info { Date, CurTemp, LowTemp, HighTemp, Done };

	virtual void showString(mil::Message m) override;
	virtual void showMain(bool force = false) override;
    virtual void showSecondary() override;
	void showInfoSequence();
	void showChars(const std::string& string, uint8_t dps, bool colon);

	Info _info = Info::Done;
	Ticker _showInfoTimer;
    
    std::unique_ptr<mil::Clock> _clock;
    
    uint8_t _lastHour = 0;
    uint8_t _lastMinute = 0;
    uint8_t _lastDps = 0;
};
