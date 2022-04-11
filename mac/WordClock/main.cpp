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

#include "tigr.h"

static constexpr const char* ImageName = "WordClock-Hoefler-800.png";
static constexpr int originX = 0;
static constexpr int originY = 5;
static constexpr int sizeX = 50;
static constexpr int sizeY = 50;
static constexpr bool showBoxes = false;
    

class WordBoard
{
public:
    enum class Word {
        ULDot, URDot, LLDot, LRDot, 
        
        Its, Half, A, Quarter, 
        TwentyMinute, FiveMinute, TenMinute, 
        
        Past, To, 
        
        OneHour, TwoHour, ThreeHour, FourHour, FiveHour, SixHour,
        SevenHour, EightHour, NineHour, TenHour, ElevenHour, Noon, Midnight,
        
        OClock, At, Night, In, The, Morning, Afternoon, Evening, 
        
        Itll, Be, Clear, Windy, Partly, Cloudy, Rainy, Snowy, 
        And, Cold, Cool, Warm, Hot,
        
        Connect, Connecting, ConnTo, Restart, Hotspot, Reset, Network,
    };
    
    enum class WeatherCondition { Clear, Windy, Cloudy, PartlyCloudy, Rainy, Snowy };
    enum class WeatherTemp { Cold, Cool, Warm, Hot };
    
    struct WordRange
    {
        WordRange(uint8_t s, uint8_t c) : start(s), count(c) { }
        uint8_t start;
        uint8_t count;
    };
    
    const WordRange& rangeFromWord(Word word)
    {
        switch (word) {
            default:
            case Word::ULDot:           { static WordRange buf(0, 1); return buf; };
            case Word::URDot:           { static WordRange buf(15, 1); return buf; };
            case Word::LLDot:           { static WordRange buf(240, 1); return buf; };
            case Word::LRDot:           { static WordRange buf(255, 1); return buf; };
            case Word::Its:             { static WordRange buf(1, 4); return buf; };
            case Word::A:               { static WordRange buf(6, 1); return buf; };
            case Word::Quarter:         { static WordRange buf(8, 7); return buf; };
            case Word::Half:            { static WordRange buf(32, 4); return buf; };
            case Word::Past:            { static WordRange buf(37, 4); return buf; };
            case Word::To:              { static WordRange buf(40, 2); return buf; };
            case Word::FiveMinute:      { static WordRange buf(23, 4); return buf; };
            case Word::TenMinute:       { static WordRange buf(27, 3); return buf; };
            case Word::TwentyMinute:    { static WordRange buf(16, 6); return buf; };
            case Word::OneHour:         { static WordRange buf(48, 3); return buf; };
            case Word::TwoHour:         { static WordRange buf(51, 3); return buf; };
            case Word::ThreeHour:       { static WordRange buf(54, 5); return buf; };
            case Word::FourHour:        { static WordRange buf(43, 4); return buf; };
            case Word::FiveHour:        { static WordRange buf(64, 4); return buf; };
            case Word::SixHour:         { static WordRange buf(92, 3); return buf; };
            case Word::SevenHour:       { static WordRange buf(59, 5); return buf; };
            case Word::EightHour:       { static WordRange buf(67, 5); return buf; };
            case Word::NineHour:        { static WordRange buf(88, 4); return buf; };
            case Word::TenHour:         { static WordRange buf(86, 3); return buf; };
            case Word::ElevenHour:      { static WordRange buf(80, 6); return buf; };
            case Word::Noon:            { static WordRange buf(119, 4); return buf; };
            case Word::Midnight:        { static WordRange buf(72, 8); return buf; };
            case Word::OClock:          { static WordRange buf(96, 7); return buf; };
            case Word::At:              { static WordRange buf(112, 2); return buf; };
            case Word::Night:           { static WordRange buf(122, 5); return buf; };
            case Word::In:              { static WordRange buf(104, 2); return buf; };
            case Word::The:             { static WordRange buf(107, 3); return buf; };
            case Word::Morning:         { static WordRange buf(128, 7); return buf; };
            case Word::Afternoon:       { static WordRange buf(114, 9); return buf; };
            case Word::Evening:         { static WordRange buf(135, 7); return buf; };
            case Word::Itll:            { static WordRange buf(144, 5); return buf; };
            case Word::Be:              { static WordRange buf(150, 2); return buf; };
            case Word::Clear:           { static WordRange buf(167, 5); return buf; };
            case Word::Windy:           { static WordRange buf(153, 5); return buf; };
            case Word::Partly:          { static WordRange buf(160, 6); return buf; };
            case Word::Cloudy:          { static WordRange buf(181, 6); return buf; };
            case Word::Rainy:           { static WordRange buf(171, 5); return buf; };
            case Word::Snowy:           { static WordRange buf(176, 5); return buf; };
            case Word::And:             { static WordRange buf(188, 3); return buf; };
            case Word::Cold:            { static WordRange buf(192, 4); return buf; };
            case Word::Cool:            { static WordRange buf(196, 4); return buf; };
            case Word::Warm:            { static WordRange buf(200, 4); return buf; };
            case Word::Hot:             { static WordRange buf(204, 3); return buf; };
            case Word::Connect:         { static WordRange buf(208, 7); return buf; };
            case Word::Connecting:      { static WordRange buf(208, 13); return buf; };
            case Word::ConnTo:          { static WordRange buf(221, 2); return buf; };
            case Word::Restart:         { static WordRange buf(224, 8); return buf; };
            case Word::Hotspot:         { static WordRange buf(232, 7); return buf; };
            case Word::Reset:           { static WordRange buf(241, 5); return buf; };
            case Word::Network:         { static WordRange buf(247, 8); return buf; };
        }
    }
    
    WordBoard()
    {
        words = tigrLoadImage(ImageName);
        screen = tigrWindow(words->w, words->h, "Hello", TIGR_AUTO);
    }
    
    void setTime(int time)
    {
        setWord(WordBoard::Word::Its);

        // Set minute dots
        switch(time % 5) {
            default:
            case 0: break;
            case 4: setWord(WordBoard::Word::LLDot);
            case 3: setWord(WordBoard::Word::LRDot);
            case 2: setWord(WordBoard::Word::URDot);
            case 1: setWord(WordBoard::Word::ULDot); break;
        }
        
        // Split out hours and minutes
        int hour = time / 60;
        int minute = (time % 60) / 5;
        
        // We don't display some things at noon and midnight
        int hourToDisplay = hour;
        if (minute > 6) {
            hourToDisplay++;
        }
        
        bool isTwelve = hourToDisplay == 0 || hourToDisplay == 12 || hourToDisplay == 24;
        
        // Do time of day
        if (!isTwelve) {
            if (hourToDisplay >= 20) {
                setWord(WordBoard::Word::At);
                setWord(WordBoard::Word::Night);
            } else {
                setWord(WordBoard::Word::In);
                setWord(WordBoard::Word::The);
                if (hourToDisplay <= 11) {
                    setWord(WordBoard::Word::Morning);
                } else if (hourToDisplay <= 16) {
                    setWord(WordBoard::Word::Afternoon);
                } else {
                    setWord(WordBoard::Word::Evening);
                }
            }
        }
        
        // Do hour
        switch(hourToDisplay % 12) {
            case 1: setWord(WordBoard::Word::OneHour); break;
            case 2: setWord(WordBoard::Word::TwoHour); break;
            case 3: setWord(WordBoard::Word::ThreeHour); break;
            case 4: setWord(WordBoard::Word::FourHour); break;
            case 5: setWord(WordBoard::Word::FiveHour); break;
            case 6: setWord(WordBoard::Word::SixHour); break;
            case 7: setWord(WordBoard::Word::SevenHour); break;
            case 8: setWord(WordBoard::Word::EightHour); break;
            case 9: setWord(WordBoard::Word::NineHour); break;
            case 10: setWord(WordBoard::Word::TenHour); break;
            case 11: setWord(WordBoard::Word::ElevenHour); break;
            case 12:
            case 0: setWord((hourToDisplay == 0 || hourToDisplay == 24) ? WordBoard::Word::Midnight : WordBoard::Word::Noon); break;
        }
        
        // Do minutes
        if (minute == 0) {
            if (!isTwelve) {
                setWord(WordBoard::Word::OClock);
            }
        } else {
            if (minute <= 6) {
                setWord(WordBoard::Word::Past);
            } else {
                setWord(WordBoard::Word::To);
                minute = 12 - minute;
            }
            switch(minute) {
                case 1: setWord(WordBoard::Word::FiveMinute); break;
                case 2: setWord(WordBoard::Word::TenMinute); break;
                case 3: setWord(WordBoard::Word::A); setWord(WordBoard::Word::Quarter); break;
                case 4: setWord(WordBoard::Word::TwentyMinute); break;
                case 5: setWord(WordBoard::Word::TwentyMinute); setWord(WordBoard::Word::FiveMinute); break;
                case 6: setWord(WordBoard::Word::Half); break;
            }
        }
    }
    
    void setWeather(WeatherCondition cond, WeatherTemp temp)
    {
        setWord(WordBoard::Word::Itll);
        setWord(WordBoard::Word::Be);
        
        switch (cond) {
            case WeatherCondition::Clear: setWord(WordBoard::Word::Clear); break;
            case WeatherCondition::Cloudy: setWord(WordBoard::Word::Cloudy); break;
            case WeatherCondition::PartlyCloudy: setWord(WordBoard::Word::Partly); setWord(WordBoard::Word::Cloudy); break;
            case WeatherCondition::Rainy: setWord(WordBoard::Word::Rainy); break;
            case WeatherCondition::Snowy: setWord(WordBoard::Word::Snowy); break;
            case WeatherCondition::Windy: setWord(WordBoard::Word::Windy); break;
        }
        
        setWord(WordBoard::Word::And);

        switch (temp) {
            case WeatherTemp::Cold: setWord(WordBoard::Word::Cold); break;
            case WeatherTemp::Cool: setWord(WordBoard::Word::Cool); break;
            case WeatherTemp::Warm: setWord(WordBoard::Word::Warm); break;
            case WeatherTemp::Hot: setWord(WordBoard::Word::Hot); break;
        }
    }
    
    void init()
    {
        memset(lights, 0, 256);
    }
    
    void update()
    {
        tigrBlit(screen, words, 0, 0, 0, 0, words->w, words->h);
        for (int i = 0; i < 256; ++i) {
            if (lights[i] == 0) {
                tigrFillRect(screen, (i % 16) * sizeX + originX, (i / 16) * sizeY + originY, sizeX, sizeY, tigrRGBA(0x00, 0x00, 0x00, 0xff));
            }
        }
        tigrUpdate(screen);
    }
    
    bool done() { return tigrClosed(screen); }
    
    void finish() { tigrFree(screen); }
    
    void setWord(Word word)
    {
        const WordRange& range = rangeFromWord(word);
        for (int i = range.start; i < range.start + range.count; ++i) {
            lights[i] = 0xff;
        }
    }
    
    void setBoxes()
    {
        tigrBlit(screen, words, 0, 0, 0, 0, words->w, words->h);
        
        for (int i = 0; i < 256; ++i) {
            tigrRect(screen, (i % 16) * sizeX + originX, (i / 16) * sizeY + originY, sizeX, sizeY, tigrRGBA(0xff, 0x00, 0x00, 0xff));
        }

        tigrUpdate(screen);
    }
    
private:
    Tigr* words = nullptr;
    Tigr* screen = nullptr;
    
    uint8_t lights[256];
};

static uint64_t startTime = 0;
static float now()
{
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    uint64_t t = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    if (startTime == 0) {
        startTime = t;
    }
    return float(t - startTime) / 1000000;
}

int main(int argc, const char * argv[])
{
    WordBoard board;
    
    int minute = 60 * 22;
    
    float rate = 0.125;
    
    float lastUpdateTime = now();
    bool needUpdate = true;

    while (!board.done())
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

            board.init();

            if (!showBoxes) {
                board.setTime(minute);
                board.setWeather(WordBoard::WeatherCondition::Clear, WordBoard::WeatherTemp::Cool);
                board.update();
            } else {
                board.setBoxes();
            }
        }
    }
    board.finish();

    return 0;
}
