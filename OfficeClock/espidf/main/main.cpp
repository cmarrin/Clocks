/*-------------------------------------------------------------------------
    This source file is a part of Office Clock
    For the latest info, see https://github.com/cmarrin/Clocks
    Copyright (c) 2021-2024, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

// Adapting the ESP32C6 Super Mini for Office Clock
//
// Office Clock was designed for the ESP8266 D1 Mini board. I'm using an
// Adaptor board  which plugs into the D1 Mini slot and wires the connections
// to the Super Mini.
//
// Here are the adaptor connections:
//      (on both boards, pin # is starting at the top on the left (L) or right (R)
//      with the board oriented with the USB connector at the top looking from the
//      ESP chip side)
//
//      Function    D1 Mini id (pin #)      Super Mini id (pin #)
//
//      5v              5v   (L1)               5v   (R1)
//      3.3v            3.3v (R1)               3.3v (R3)
//      Gnd             Gnd  (L2)               Gnd  (R2)
//      A0              A0   (R7)               1    (L4)
//      MOSI            D7   (R3)               4    (L7)
//      CLK             D5   (R5)               3    (L6)
//      CS              D8   (R2)               7    (L10)
//      Button          D1   (L6)               18   (R8)

#include "OfficeClock.h"

#include "IDFWiFiPortal.h"

mil::IDFWiFiPortal portal;

static const char* TAG = "OfficeClock";

extern "C" {
void app_main(void)
{
    System::logI(TAG, "Starting OfficeClock...");
    OfficeClock officeClock(&portal, false);
    officeClock.setup();

    while (true) {
        officeClock.loop();
        vTaskDelay(1);
    }
}
}
