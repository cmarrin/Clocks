//
//  main.cpp
//  WordClock
//
//  Created by Chris Marrin on 4/5/22.
//

#define GL_SILENCE_DEPRECATION

#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <OpenGL/gl.h>
#include <chrono>

#include "WordClock.h"
#include "tigr.h"

static constexpr const char* ImageName = "WordClock-Hoefler-800.png";
static constexpr int originX = 0;
static constexpr int originY = 5;
static constexpr int sizeX = 50;
static constexpr int sizeY = 50;
static constexpr bool showBoxes = false;
    

static float now()
{
    static uint64_t startTime = 0;
    
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    uint64_t t = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    if (startTime == 0) {
        startTime = t;
    }
    return float(t - startTime) / 1000000;
}

void setBoxes(Tigr* words, Tigr* screen)
{
    tigrBlit(screen, words, 0, 0, 0, 0, words->w, words->h);
    
    for (int i = 0; i < 256; ++i) {
        tigrRect(screen, (i % 16) * sizeX + originX, (i / 16) * sizeY + originY, sizeX, sizeY, tigrRGBA(0xff, 0x00, 0x00, 0xff));
    }

    tigrUpdate(screen);
}
    
int main(int argc, const char * argv[])
{
    WordClock clock;

    Tigr* words = tigrLoadImage(ImageName);
    Tigr* screen = tigrWindow(words->w, words->h, "Hello", TIGR_AUTO);
    
    int minute = 60 * 22;
    
    float rate = 0.125;
    
    float lastUpdateTime = now();
    bool needUpdate = true;

    while (!tigrClosed(screen))
    {
        float elaspedTime = now();
        if (elaspedTime - lastUpdateTime > rate) {
            needUpdate = true;
            lastUpdateTime = elaspedTime;
            if (++minute >= 60 * 24) {
                minute = 0;
            }
        }
            
        if (needUpdate) {
            needUpdate = false;

            clock.init();

            if (!showBoxes) {
                clock.setTime(minute);
                clock.setWeather(WordClock::WeatherCondition::Clear, WordClock::WeatherTemp::Cool);

                const uint8_t* lights = clock.lightState();
                tigrBlit(screen, words, 0, 0, 0, 0, words->w, words->h);
                for (int i = 0; i < 256; ++i) {
                    if (lights[i] == 0) {
                        tigrFillRect(screen, (i % 16) * sizeX + originX, (i / 16) * sizeY + originY, sizeX, sizeY, tigrRGBA(0x00, 0x00, 0x00, 0xff));
                    }
                }
                tigrUpdate(screen);
            } else {
                setBoxes(words, screen);
            }
        }
    }
    tigrFree(screen);
    
    return 0;
}
