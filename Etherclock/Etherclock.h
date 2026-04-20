/*-------------------------------------------------------------------------
    This source file is a part of Etherclock
    For the latest info, see https://github.com/cmarrin/Clocks
    Copyright (c) 2021-2026, Chris Marrin
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
#include "BrightnessManager.h"
#include "ButtonManager.h"
#include "DSP7S04B.h"

static constexpr const char* ConfigPortalName = "MT Etherclock";
static constexpr const char* Hostname = "officeclock";
static constexpr const char* Version = "5.0";

// Display related
// Range for the light sensor on the ESP23C6 was measured at 3200 in 
// a dark room and 200 with a light shining on the sensor. This is
// an inverted range so we invert it before applying the min and max. Also
// The incoming value is scaled to 10 bits, so the min and max
// values are based on that.

static constexpr uint8_t SelectButton = 14;
static constexpr uint32_t LightSensor = 1;
static constexpr uint32_t NumberOfBrightnessLevels = 250;
static constexpr bool InvertAmbientLightLevel = false;
static constexpr uint32_t MinLightSensorLevel = 0;
static constexpr uint32_t MaxLightSensorLevel = 300;

static constexpr uint32_t SecondaryTimePerInfo = 2000; // In ms

class Etherclock : public mil::Application
{
public:
	Etherclock(mil::WiFiPortal*, bool buttonActiveHigh, mil::RenderCB = nullptr);
	
	virtual void setup() override;
	virtual void loop() override;

private:
    enum class Info { Day, Date, CurTemp, LowTemp, HighTemp, Done };

    void handleButtonEvent(const mil::Button& button, mil::ButtonManager::Event event);
    
    void setBrightness(uint8_t b)
    {
        // FIXME: Set a low light level until light sensor is hooked up
        _clockDisplay.setBrightness(b);
    }
    
    virtual void showString(mil::Message m) override;
	virtual void showMain(bool force = false) override;
    virtual void showSecondary() override;
	void showInfoSequence();
	void showChars(const char* string, uint8_t dps, bool colon);

    mil::DSP7S04B _clockDisplay;

	Info _info = Info::Done;
	mil::Ticker _showInfoTimer;
    
	mil::BrightnessManager _brightnessManager;
	mil::ButtonManager _buttonManager;
    bool _buttonActiveHigh = false;

    uint8_t _lastHour = 0;
    uint8_t _lastMinute = 0;
    uint8_t _lastDps = 0;
};
