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

#include "Etherclock.h"

int main(int argc, const char * argv[])
{
    Etherclock etherclock;
    
    etherclock.setup();
    
    while (true) {
        etherclock.loop();
    }
    return 0;
}
