//
//  WordClock.h
//
//  Created by Chris Marrin on 4/13/22.
//

#pragma once

#include <cstdint>
#include <cstring>

// WordClock class.
//
// This class manages a board of characters which lights up to show the time and weather in words.
// The board is arranged as a 16x16 array of these characters:
//
//      • I T ' S O A O Q U A R T E R •
//      T W E N T Y O F I V E T E N O O
//      H A L F O P A S T O O F O U R O
//      O N E T W O T H R E E S E V E N
//      F I V E I G H T M I D N I G H T
//      E L E V E N T E N I N E S I X O
//      O ' C L O C K O I N O T H E O O
//      A T A F T E R N O O N I G H T O
//      M O R N I N G E V E N I N G O O
//      I T ' L L O B E O W I N D Y O O
//      P A R T L Y O C L E A R A I N Y
//      S N O W Y C L O U D Y O A N D O
//      C O L D C O O L W A R M H O T O
//      C O N N E C T I N G . . . T O O
//      R E S T A R T O H O T S P O T O
//      • R E S E T O N E T W O R K O •

class WordClock
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
    
    const WordRange& rangeFromWord(Word word) const;

    WordClock() { }
    
    void init() { memset(lights, 0, 256); }
    
    // Return a 256 element byte array with the brightness of each light
    const uint8_t* lightState() const { return lights; }
    
    void setTime(int time);
    
    void setWeather(WeatherCondition cond, WeatherTemp temp);
    
private:
    void setWord(Word word)
    {
        const WordRange& range = rangeFromWord(word);
        for (int i = range.start; i < range.start + range.count; ++i) {
            lights[i] = 0xff;
        }
    }
    
    uint8_t lights[256];
};

