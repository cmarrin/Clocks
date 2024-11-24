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

#include <sys/ioctl.h>

static int getc()
{
    int bytes = 0;
    if (ioctl(0, FIONREAD, &bytes) == -1) {
        return -1;
    }
    if (bytes == 0) {
        return 0;
    }
    return getchar();
}

int main(int argc, const char * argv[])
{
    system("stty raw");

    Etherclock etherclock;
    
    etherclock.setup();
    
    while (true) {
        etherclock.loop();

        int c = getc();
        if (c == '1') {
            cout << " Got Click\n";
            etherclock.sendInput(mil::Input::Click);
        } else if (c == '2') {
            cout << " Got Long Press\n";
            etherclock.sendInput(mil::Input::LongPress);
        }
    }
    return 0;
}
