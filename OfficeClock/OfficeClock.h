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
#include "Clock.h"
#include "BrightnessManager.h"
#include "ButtonManager.h"

#ifdef ARDUINO
#include "Max7219Display.h"
#endif

static constexpr const char* ConfigPortalName = "MT Etherclock";
static constexpr const char* Hostname = "officeclock";

// Display related
static constexpr uint32_t StartupScrollRate = 50;
static constexpr uint32_t DateScrollRate = 50;
static constexpr uint8_t SelectButton = 5;
static constexpr uint32_t LightSensor = 0;
static constexpr uint32_t NumberOfBrightnessLevels = 31;
static constexpr bool InvertAmbientLightLevel = true;
static constexpr uint32_t MinLightSensorLevel = 50;
static constexpr uint32_t MaxLightSensorLevel = 900;

// The Mac simulator calls the showDoneTimer immediately after displaying
// a scrolling message so a 100ms duration causes it to show the message
// a lot. On the hardware it doesn't call it until after the scroll is
// finished. This causes the scroll to repeat right away so 100ms is
// appropriate. Use the most reasonable value for each.
#ifdef ARDUINO
static constexpr uint32_t DoneTimeDuration = 100;
#else
static constexpr uint32_t DoneTimeDuration = 5000;
#endif

class OfficeClock : public mil::Application
{
  public:
    OfficeClock();

    virtual void setup() override;
    virtual void loop() override;

  private:	
    virtual void showMain(bool force) override;
    virtual void showSecondary() override;
    virtual void showString(mil::Message m) override;

    void handleButtonEvent(const mil::Button& button, mil::ButtonManager::Event event);
    void setBrightness(uint32_t b);

    mil::Max7219Display _clockDisplay;
    std::unique_ptr<mil::Clock> _clock;
    mil::BrightnessManager _brightnessManager;
    mil::ButtonManager _buttonManager;

    String _lastStringSent;
};
