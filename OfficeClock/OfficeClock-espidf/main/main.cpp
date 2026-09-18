/*-------------------------------------------------------------------------
    This source file is a part of Office Clock
    For the latest info, see https://github.com/cmarrin/Clocks
    Copyright (c) 2021-2024, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

// ESP32-C6 Super Mini for Office Clock
//
// Clock consists of a MAX7219 based LED matrix (both 24x8 and 32x8 supported).
// A 74HCT367 is used to level shift from 3.3v on the ESP32-C6 to 5v on the
// LED matrix. A single button is connected between the ESP32-C6 and ground.
// A TEMT6000 light sensor on a breakout board is used for dimming. It uses
// 3.3v, ground and a sense line connected to the ADC on the ESP32-C6.
//
// Here are the connections:
//      for the ESP32-C6 pin # is starting at the top right with the board 
//      oriented with the USB connector at the top looking from the ESP chip
//      side. Numbering goes counter-clockwise so pin 20 is top left
//
//      Function    Super Mini id (pin #)
//
//      5v              5v   (20)   - LED Matrix VCC
//      3.3v            3.3v (18)   - Light Sensor VCC
//      Gnd             Gnd  (19)   - LED Matrix GND, Light Sensor GND, Button GND
//      A0              1    ( 4)   - Light Sensor SENSE
//      MOSI            4    ( 7)   - LED Matrix DIN
//      CLK             3    ( 6)   - LED Matrix CLK
//      CS              7    (10)   - LEDMatrix CS
//      Button          14   (13)   - Button

#include "OfficeClock.h"

#include "IDFWiFiPortal.h"

mil::IDFWiFiPortal portal;

static const char* TAG = "OfficeClock";

extern "C" {
void app_main(void)
{
    mil::System::logI(TAG, "Starting OfficeClock...");
    OfficeClock officeClock(&portal, false);
    officeClock.setup();

    while (true) {
        officeClock.loop();
        vTaskDelay(1);
    }
}
}
