/*-------------------------------------------------------------------------
    This source file is a part of Etherclock
    For the latest info, see https://github.com/cmarrin/Clocks
    Copyright (c) 2021-2026, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

// Adapting the ESP32C6 Super Mini for Etherclock
//
// Etherclock was designed for the ESP8266. I'm rebuilding it using the same
// DSP7S04B display, light sensor and button. I'll use the same pin connections
// as Office Clock:
//      Function    Super Mini id (pin #)
//
//      5v              5v       (R1)
//      3.3v            3.3v     (R3)
//      Gnd             Gnd      (R2)
//      A0              GPIO1    (L4)
//      MOSI            GPIO4    (L7)
//      CLK             GPIO3    (L6)
//      CS              GPIO7    (L10)
//      Button          GPIO14   (R8)

#include "Etherclock.h"

#include "IDFWiFiPortal.h"

mil::IDFWiFiPortal portal;

static const char* TAG = "Etherclock";

extern "C" {
void app_main(void)
{
    mil::System::logI(TAG, "Starting Etherclock...");
    Etherclock etherclock(&portal, false);
    etherclock.setup();

    while (true) {
        etherclock.loop();
        vTaskDelay(1);
    }
}
}
