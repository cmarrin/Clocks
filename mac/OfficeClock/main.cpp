/*-------------------------------------------------------------------------
    This source file is a part of Office Clock
    For the latest info, see https://github.com/cmarrin/Clocks
    Copyright (c) 2021-2024, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "OfficeClock.h"

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

    OfficeClock officeClock;
    
    officeClock.setup();
    
    while (true) {
        officeClock.loop();

        int c = getc();
        if (c == '1') {
            cout << " Got Click\n";
            officeClock.sendInput(mil::Input::Click, true);
        } else if (c == '2') {
            cout << " Got Long Press\n";
            officeClock.sendInput(mil::Input::LongPress, true);
        }
    }
    return 0;
}
