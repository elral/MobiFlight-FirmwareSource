//
// DigInMux.h
//
// (C) MobiFlight Project 2022
//


#pragma once

#include <Arduino.h>

namespace DigInMux
{
    bool setupArray(uint16_t count);
    uint8_t Add(uint8_t dataPin, uint8_t nRegs, char const *name = "DigInMux");
    void Clear();
    void read();
    void OnTrigger();
}

// DigInMux.h