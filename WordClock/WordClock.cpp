//
//  WordClock.cpp
//  Clocks
//
//  Created by Chris Marrin on 4/13/22.
//

#include "WordClock.h"

const WordClock::WordRange&
WordClock::rangeFromWord(Word word) const
{
    switch (word) {
        default:
        case Word::ULDot:           { static WordRange buf(0, 1); return buf; };
        case Word::URDot:           { static WordRange buf(15, 1); return buf; };
        case Word::LLDot:           { static WordRange buf(240, 1); return buf; };
        case Word::LRDot:           { static WordRange buf(255, 1); return buf; };
        case Word::Its:             { static WordRange buf(1, 3); return buf; };
        case Word::A:               { static WordRange buf(5, 1); return buf; };
        case Word::Quarter:         { static WordRange buf(7, 7); return buf; };
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
        case Word::OClock:          { static WordRange buf(97, 6); return buf; };
        case Word::At:              { static WordRange buf(112, 2); return buf; };
        case Word::Night:           { static WordRange buf(122, 5); return buf; };
        case Word::In:              { static WordRange buf(104, 2); return buf; };
        case Word::The:             { static WordRange buf(107, 3); return buf; };
        case Word::Morning:         { static WordRange buf(128, 7); return buf; };
        case Word::Afternoon:       { static WordRange buf(114, 9); return buf; };
        case Word::Evening:         { static WordRange buf(135, 7); return buf; };
        case Word::Itll:            { static WordRange buf(144, 4); return buf; };
        case Word::Be:              { static WordRange buf(149, 2); return buf; };
        case Word::Clear:           { static WordRange buf(167, 5); return buf; };
        case Word::Windy:           { static WordRange buf(152, 5); return buf; };
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

void
WordClock::setTime(int time)
{
    setWord(Word::Its);

    // Set minute dots
    switch(time % 5) {
        default:
        case 0: break;
        case 4: setWord(Word::LLDot);
        case 3: setWord(Word::LRDot);
        case 2: setWord(Word::URDot);
        case 1: setWord(Word::ULDot); break;
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
            setWord(Word::At);
            setWord(Word::Night);
        } else {
            setWord(Word::In);
            setWord(Word::The);
            if (hourToDisplay <= 11) {
                setWord(Word::Morning);
            } else if (hourToDisplay <= 16) {
                setWord(Word::Afternoon);
            } else {
                setWord(Word::Evening);
            }
        }
    }
    
    // Do hour
    switch(hourToDisplay % 12) {
        case 1: setWord(Word::OneHour); break;
        case 2: setWord(Word::TwoHour); break;
        case 3: setWord(Word::ThreeHour); break;
        case 4: setWord(Word::FourHour); break;
        case 5: setWord(Word::FiveHour); break;
        case 6: setWord(Word::SixHour); break;
        case 7: setWord(Word::SevenHour); break;
        case 8: setWord(Word::EightHour); break;
        case 9: setWord(Word::NineHour); break;
        case 10: setWord(Word::TenHour); break;
        case 11: setWord(Word::ElevenHour); break;
        case 12:
        case 0: setWord((hourToDisplay == 0 || hourToDisplay == 24) ? Word::Midnight : Word::Noon); break;
    }
    
    // Do minutes
    if (minute == 0) {
        if (!isTwelve) {
            setWord(Word::OClock);
        }
    } else {
        if (minute <= 6) {
            setWord(Word::Past);
        } else {
            setWord(Word::To);
            minute = 12 - minute;
        }
        switch(minute) {
            case 1: setWord(Word::FiveMinute); break;
            case 2: setWord(Word::TenMinute); break;
            case 3: setWord(Word::A); setWord(Word::Quarter); break;
            case 4: setWord(Word::TwentyMinute); break;
            case 5: setWord(Word::TwentyMinute); setWord(Word::FiveMinute); break;
            case 6: setWord(Word::Half); break;
        }
    }
}

void
WordClock::setWeather(WeatherCondition cond, WeatherTemp temp)
{
    setWord(Word::Itll);
    setWord(Word::Be);
    
    switch (cond) {
        case WeatherCondition::Clear: setWord(Word::Clear); break;
        case WeatherCondition::Cloudy: setWord(Word::Cloudy); break;
        case WeatherCondition::PartlyCloudy: setWord(Word::Partly); setWord(Word::Cloudy); break;
        case WeatherCondition::Rainy: setWord(Word::Rainy); break;
        case WeatherCondition::Snowy: setWord(Word::Snowy); break;
        case WeatherCondition::Windy: setWord(Word::Windy); break;
    }
    
    setWord(Word::And);

    switch (temp) {
        case WeatherTemp::Cold: setWord(Word::Cold); break;
        case WeatherTemp::Cool: setWord(Word::Cool); break;
        case WeatherTemp::Warm: setWord(Word::Warm); break;
        case WeatherTemp::Hot: setWord(Word::Hot); break;
    }
}
