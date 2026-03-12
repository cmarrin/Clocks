/*-------------------------------------------------------------------------
    This source file is a part of Office Clock
    For the latest info, see https://github.com/cmarrin/Clocks
    Copyright (c) 2021-2024, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "OfficeClock.h"

#include "IDFWiFiPortal.h"

mil::IDFWiFiPortal portal;

static const char* TAG = "OfficeClock";

extern "C" {
void app_main(void)
{
    System::logI(TAG, "Starting OfficeClock...");
    OfficeClock officeClock(&portal);
    officeClock.setup();

    while (true) {
        officeClock.loop();
        vTaskDelay(1);
    }
}
}
