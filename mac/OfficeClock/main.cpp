/*-------------------------------------------------------------------------
    This source file is a part of Office Clock
    For the latest info, see https://github.com/cmarrin/Clocks
    Copyright (c) 2021-2024, Chris Marrin
    All rights reserved.
    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "OfficeClock.h"

#include "MacWiFiPortal.h"
#include "tigr.h"

mil::MacWiFiPortal portal;

static const char* TAG = "OfficeClock";
static constexpr int LEDBorder = 1;
static constexpr int LEDRadius = 5;
static constexpr int Offset = 10;
static constexpr int MessageHeight = 20;
static constexpr int Spacing = ((LEDBorder + LEDRadius) * 2);
static constexpr int MatrixWidth = Spacing * 32 + (Offset * 2);
static constexpr int MatrixHeight = Spacing * 8 + (Offset * 2);
static constexpr int WindowWidth = MatrixWidth;
static constexpr int WindowHeight = MatrixHeight + MessageHeight;
static constexpr int MessageX = 100;
static constexpr int MessageY = WindowHeight - 20;

int main(int argc, const char * argv[])
{
    while (true) {
        mil::System::logI(TAG, "Opening tigr window");

        Tigr* screen = tigrWindow(WindowWidth, WindowHeight, "Hello", TIGR_AUTO);
        
        OfficeClock officeClock(&portal, true, [screen](const mil::Graphics* gfx)
        {
            tigrClear(screen, tigrRGBA(0x0, 0x00, 0x00, 0xff));

            // Show the button text
            tigrPrint(screen, tfont, MessageX, MessageY, tigrRGB(0xff, 0xff, 0xff), "Press [TAB] for select");

            // Make a vertical grid
            for (int i = 0; i <= 32; i++) {
                uint8_t color = ((i % 8) == 0) ? 0x50 : 0x30;
                tigrLine(screen, Offset + (i * Spacing), Spacing, Offset + (i * Spacing), MatrixHeight - Spacing + 1, tigrRGBA(color, color, color, 0xff));
            }
            
            // Make a horizontal grid
            for (int i = 0; i <= 8; i++) {
                tigrLine(screen, Offset, Offset + (i * Spacing), MatrixWidth - Spacing + 1, Offset + (i * Spacing), tigrRGBA(0x30, 0x30, 0x30, 0xff));
            }
            
            int x = 0;
            int y = 0;
            const uint32_t* buffer = reinterpret_cast<const uint32_t*>(gfx->getBuffer());

            for (int i = 0; i < 32; ++i) {
                uint8_t c = buffer[i];
                for (int j = 0; j < 8; ++j) {
                    if (c & 0x80) {
                        tigrFillCircle(screen, x + Offset + LEDRadius, y + Offset + LEDRadius, LEDRadius, tigrRGBA(0xff, 0x00, 0x00, 0xff));
                    }
                    c <<= 1;
                    x += Spacing;
                }
                if ((i % 4) == 3) {
                    x = 0;
                    y += Spacing;
                }
            }
        });
        
        officeClock.setup();
        
        while (!tigrClosed(screen) && !tigrKeyDown(screen, TK_ESCAPE)) {
            if (tigrKeyDown(screen, TK_TAB) || tigrKeyHeld(screen, TK_TAB)) {
                mil::System::setButtonDown(true);
            } else {
                mil::System::setButtonDown(false);
            }
            
            if (mil::System::isRestarting()) {
                break;
            }
            
            officeClock.loop();
            tigrUpdate(screen);
            mil::System::delay(10);
        }

        tigrFree(screen);
    }
    return 0;
}
