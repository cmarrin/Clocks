/*-------------------------------------------------------------------------
    This source file is a part of Office Clock
    For the latest info, see https://github.com/cmarrin/Clocks
    Copyright (c) 2021-2024, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

// Arduino IDE Setup:
//
// Board: LOLIN(WEMOS) D1 R2 & mini

// Office Clock is an ESP8266 (Wemos D1 Mini) based digital clock. Display is a
// 32 x 8 dot matrix LED array driven by a MAX7219 over SPI. The SPI interface
// does not use MISO, so it only uses 3 pins. It also has an ambient light sensor
// on AO, and 3 switches to change functions and settings.
//
// It uses the mil::TimeWeatherServer to get the time current weather conditions,
// current, low and high temps

// Wemos Pinout
//
//      A0 - Light sensor
//
//      D1 - Select Button (active high)
//      D4 - On board LED
//      D5 - Matrix CLK
//      D7 - Matrix DIN
//      D8 - Matrix CS
//
// Light sensor
//
//                           Wemos A0
//                             |
//                             |
//		3.3v -------- 47KΩ ----|---- Sensor -------- GND
//                                   ^
//                                   |
//                             Longer Lead
//
// Button
//
//		D1 -------- Button (N/O) -------- GND
// 74HCT367
//
// 		Since the MAX7219 is a 5v part and the ESP8266 is 3.3v, a 74HCT367 is used to adapt
// 		the voltage levels:
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
//			9		N/C
//			10		GND
//			11		N/C
//			12		GND
//			13		N/C
//			14		GND
//			15		GND
//			16		5V
//
// Power
//
//		Connect: Wemos 5v, 74HCT367 pin 16, Max7219 Vcc
//		Connect: Wemos 3.3v, Light sensor (through 47KΩ resistor)
//		Connect: Wemos GND, 74HCT367 (pins 1, 8, 10, 12, 14, 15), Light sensor (shorter lead), Max7219 GND, One end of Button

#include "OfficeClock.h"

OfficeClock officeClock;

void setup()
{
    Serial.begin(115200);
	officeClock.setup();
}

void loop()
{
	officeClock.loop();
}

