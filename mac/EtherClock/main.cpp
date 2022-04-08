//
//  main.cpp
//  EtherClock
//
//  Created by Chris Marrin on 4/5/22.
//

#include <iostream>

#include "tigr.h"

int main(int argc, const char * argv[])
{
    Tigr* screen = tigrWindow(800, 800, "Hello", TIGR_AUTO);
    Tigr* words = tigrLoadImage("words.png");

    while (!tigrClosed(screen))
    {
        tigrClear(screen, tigrRGB(0xa0, 0x40, 0x40));
        tigrFillRect(screen, 300, 200, 50, 50, tigrRGBA(0x00, 0x00, 0x00, 0xc0));
        tigrUpdate(screen);
    }
    tigrFree(screen);

    return 0;
}
