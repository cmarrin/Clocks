/*-------------------------------------------------------------------------
    This source file is a part of Office Clock
    For the latest info, see https://github.com/cmarrin/Clocks
    Copyright (c) 2021-2024, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "mil.h"
#include "Application.h"
#include "BrightnessManager.h"
#include "ButtonManager.h"
#include "Max7219Display.h"

static constexpr const char* ConfigPortalName = "MT Office Clock";
static constexpr const char* Hostname = "officeclock";
static constexpr const char* Version = "4.0";

// Display related
// Range for the light sensor on the ESP23C6 was measured at 3200 in 
// a dark room and 200 with a light shining on the sensor. This is
// an inverted range so we invert it before applying the min and max. Also
// The incoming value is scaled to 10 bits, so the min and max
// values are based on that.

static constexpr uint32_t StartupScrollRate = 50;
static constexpr uint32_t DateScrollRate = 50;
static constexpr uint8_t SelectButton = 14;
static constexpr uint32_t LightSensor = 0;
static constexpr uint32_t NumberOfBrightnessLevels = 31;
static constexpr bool InvertAmbientLightLevel = true;
static constexpr uint32_t MinLightSensorLevel = 200; // based on a 10 bit (scaled) value
static constexpr uint32_t MaxLightSensorLevel = 950; // based on a 10 bit (scaled) value
static constexpr uint32_t DoneTimeDuration = 100;

class OfficeClock : public mil::Application
{
  public:
    OfficeClock(mil::WiFiPortal*, std::function<void(const uint8_t* buffer)> = nullptr);

    virtual void setup() override;
    virtual void loop() override;

  private:	
    virtual void showMain(bool force) override;
    virtual void showSecondary() override;
    virtual void showString(mil::Message m) override;

    void handleButtonEvent(const mil::Button& button, mil::ButtonManager::Event event);
    void setBrightness(uint32_t b);

    mil::Max7219Display _clockDisplay;
    mil::BrightnessManager _brightnessManager;
    mil::ButtonManager _buttonManager;

    std::string _lastStringSent;
};
