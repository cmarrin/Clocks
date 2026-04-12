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

#include "MacWiFiPortal.h"
#include "tigr.h"

mil::MacWiFiPortal portal;

static const char* TAG = "Etherclock";
static constexpr int DPRadius = 5;
static constexpr int Offset = 10;
static constexpr int MessageHeight = 20;
static constexpr int HorizSegmentWidth = 20;
static constexpr int HorizSegmentHeight = 3;
static constexpr int VertSegmentWidth = HorizSegmentHeight;
static constexpr int VertSegmentHeight = HorizSegmentWidth;
static constexpr int DPPosX = HorizSegmentWidth;
static constexpr int DPPosY = VertSegmentHeight * 2;
static constexpr int GPosX = 0;
static constexpr int GPosY = VertSegmentHeight;
static constexpr int FPosX = 0;
static constexpr int FPosY = 0;
static constexpr int EPosX = 0;
static constexpr int EPosY = VertSegmentHeight;
static constexpr int DPosX = 0;
static constexpr int DPosY = VertSegmentHeight * 2;
static constexpr int CPosX = HorizSegmentWidth;
static constexpr int CPosY = VertSegmentHeight * 2;
static constexpr int BPosX = HorizSegmentWidth;
static constexpr int BPosY = 0;
static constexpr int APosX = 0;
static constexpr int APosY = 0;

static constexpr int Spacing = HorizSegmentWidth + Offset * 2;
static constexpr int MatrixWidth = Spacing * 4 + (Offset * 2);
static constexpr int MatrixHeight = Offset * 2 + VertSegmentHeight * 2;
static constexpr int WindowWidth = MatrixWidth;
static constexpr int WindowHeight = MatrixHeight + MessageHeight;
static constexpr int MessageX = 20;
static constexpr int MessageY = WindowHeight - 20;

int main(int argc, const char * argv[])
{
    while (true) {
        mil::System::logI(TAG, "Opening tigr window");

        Tigr* screen = tigrWindow(WindowWidth, WindowHeight, "Hello", TIGR_AUTO);
        
        Etherclock etherclock(&portal, true, [screen](const mil::Graphics* gfx)
        {
            tigrClear(screen, tigrRGBA(0x0, 0x00, 0x00, 0xff));

            // Show the button text
            tigrPrint(screen, tfont, MessageX, MessageY, tigrRGB(0xff, 0xff, 0xff), "Press [TAB] for select");

            const uint32_t* buffer = reinterpret_cast<const uint32_t*>(gfx->getBuffer());
            TPixel color = tigrRGBA(0xff, 0x00, 0x00, 0xff);

            for (int i = 0; i < 4; ++i) {
                // Each digit is 2 bytes. The first byte is DP,g,f,e,d,c,b,a.
                // The second byte is ignored except for the last digit, where
                // the lsb indicates whether or not the colon is on
                uint8_t a = buffer[i * 2];
                uint8_t b = buffer[i * 2 + 1];

                if (a & 0x80) {
                    // DP
                    tigrFillCircle(screen, Spacing * i + DPPosX, DPPosY, DPRadius, color);
                }
                if (a & 0x40) {
                    // g
                    tigrFill(screen, Spacing * i + GPosX, GPosY, HorizSegmentWidth, HorizSegmentHeight, color);
                }
                if (a & 0x20) {
                    // f
                    tigrFill(screen, Spacing * i + FPosX, FPosY, VertSegmentWidth, VertSegmentHeight, color);
                }
                if (a & 0x10) {
                    // e
                    tigrFill(screen, Spacing * i + EPosX, EPosY, VertSegmentWidth, VertSegmentHeight, color);
                }
                if (a & 0x08) {
                    // d
                    tigrFill(screen, Spacing * i + DPosX, DPosY, HorizSegmentWidth, HorizSegmentHeight, color);
                }
                if (a & 0x04) {
                    // c
                    tigrFill(screen, Spacing * i + CPosX, CPosY, VertSegmentWidth, VertSegmentHeight, color);
                }
                if (a & 0x02) {
                    // b
                    tigrFill(screen, Spacing * i + BPosX, BPosY, VertSegmentWidth, VertSegmentHeight, color);
                }
                if (a & 0x01) {
                    // a
                    tigrFill(screen, Spacing * i + APosX, APosY, HorizSegmentWidth, HorizSegmentHeight, color);
                }
            }
        });
        
        etherclock.setup();
        
        while (!tigrClosed(screen) && !tigrKeyDown(screen, TK_ESCAPE)) {
            if (tigrKeyDown(screen, TK_TAB) || tigrKeyHeld(screen, TK_TAB)) {
                mil::System::setButtonDown(true);
            } else {
                mil::System::setButtonDown(false);
            }
            
            if (mil::System::isRestarting()) {
                break;
            }
            
            etherclock.loop();
            tigrUpdate(screen);
            mil::System::delay(10);
        }

        tigrFree(screen);
    }
    return 0;
}
