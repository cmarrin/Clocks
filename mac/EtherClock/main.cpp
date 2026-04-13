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
static constexpr int Offset = 15;
static constexpr int SegSpacing = 4;
static constexpr int MessageHeight = 20;
static constexpr int SegmentWidth = 30;
static constexpr int SegmentHeight = 5;

// Segments have 3 X Positions:
//
//      1 - Seg E, F left edge
//      2 - Seg A, D, G left edge
//      3 - Seg B, C left edge
static constexpr int X1Pos = 0;
static constexpr int X2Pos = X1Pos + SegSpacing;
static constexpr int X3Pos = X2Pos + SegmentWidth;

// Segments have 5 Y Positions:
//
//      1 - Seg A top edge
//      2 - Seg B, F top edge
//      3 - Seg G top edge
//      4 - Seg C, E top edge
//      5 - Seg D top edge
static constexpr int Y1Pos = 0;
static constexpr int Y2Pos = Y1Pos + SegSpacing;
static constexpr int Y3Pos = Y2Pos + SegmentWidth;
static constexpr int Y4Pos = Y3Pos + SegSpacing;
static constexpr int Y5Pos = Y4Pos + SegmentWidth;

static constexpr int APosX = X2Pos;
static constexpr int APosY = Y1Pos;
static constexpr int BPosX = X3Pos;
static constexpr int BPosY = Y2Pos;
static constexpr int CPosX = X3Pos;
static constexpr int CPosY = Y4Pos;
static constexpr int DPosX = X2Pos;
static constexpr int DPosY = Y5Pos;
static constexpr int EPosX = X1Pos;
static constexpr int EPosY = Y4Pos;
static constexpr int FPosX = X1Pos;
static constexpr int FPosY = Y2Pos;
static constexpr int GPosX = X2Pos;
static constexpr int GPosY = Y3Pos;

static constexpr int Spacing = SegmentWidth + Offset * 2;
static constexpr int MatrixWidth = Spacing * 4 + Offset * 2 + DPRadius * 2;
static constexpr int MatrixHeight = Offset * 2 + SegmentHeight * 3 + SegmentWidth * 2;
static constexpr int WindowWidth = MatrixWidth;
static constexpr int WindowHeight = MatrixHeight + MessageHeight;
static constexpr int MessageX = 70;
static constexpr int MessageY = WindowHeight - 20;

static constexpr int DPPosX = BPosX + SegSpacing * 2 + DPRadius;
static constexpr int DPPosY = DPosY + SegSpacing;
static constexpr int ColonPosX = Spacing * 2 + DPRadius;
static constexpr int Colon1PosY = Offset + SegmentHeight + SegmentWidth / 2;
static constexpr int Colon2PosY = Colon1PosY + SegmentWidth;

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

            const uint8_t* buffer = reinterpret_cast<const uint8_t*>(gfx->getBuffer());
            TPixel color = tigrRGBA(0xff, 0x00, 0x00, 0xff);

            for (int i = 0; i < 4; ++i) {
                // Each digit is 2 bytes. The first byte is DP,g,f,e,d,c,b,a.
                // The second byte is ignored except for the last digit, where
                // the lsb indicates whether or not the colon is on
                uint8_t a = buffer[i * 2];
                uint8_t b = buffer[i * 2 + 1];
                
                int startX = Offset + Spacing * i;

                if (i == 3 && b & 0x01) {
                    // Colon
                    tigrFillCircle(screen, ColonPosX, Colon1PosY, DPRadius, color);
                    tigrFillCircle(screen, ColonPosX, Colon2PosY, DPRadius, color);
                }
                if (a & 0x80) {
                    // DP
                    tigrFillCircle(screen, startX + DPPosX, Offset + DPPosY, DPRadius, color);
                }
                if (a & 0x40) {
                    // g
                    tigrFill(screen, startX + GPosX, Offset + GPosY, SegmentWidth, SegmentHeight, color);
                }
                if (a & 0x20) {
                    // f
                    tigrFill(screen, startX + FPosX, Offset + FPosY, SegmentHeight, SegmentWidth, color);
                }
                if (a & 0x10) {
                    // e
                    tigrFill(screen, startX + EPosX, Offset + EPosY, SegmentHeight, SegmentWidth, color);
                }
                if (a & 0x08) {
                    // d
                    tigrFill(screen, startX + DPosX, Offset + DPosY, SegmentWidth, SegmentHeight, color);
                }
                if (a & 0x04) {
                    // c
                    tigrFill(screen, startX + CPosX, Offset + CPosY, SegmentHeight, SegmentWidth, color);
                }
                if (a & 0x02) {
                    // b
                    tigrFill(screen, startX + BPosX, Offset + BPosY, SegmentHeight, SegmentWidth, color);
                }
                if (a & 0x01) {
                    // a
                    tigrFill(screen, startX + APosX, Offset + APosY, SegmentWidth, SegmentHeight, color);
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
