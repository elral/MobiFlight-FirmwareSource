//
// Analog.h
//
// (C) MobiFlight Project 2022
//

#pragma once

#include <Arduino.h>

namespace Analog
{
    bool    setupArray(uint16_t count);
    uint8_t Add(uint8_t pin, char const *name = "AnalogInput", uint8_t sensitivity = 3, bool deprecated = true);
    void    Clear();
    void    read();
    void    readAverage();
    void    OnTrigger();
    int16_t getActualValue(uint8_t channel);
}

// Analog.h