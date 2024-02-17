//
// DigInMux.h
//
// (C) MobiFlight Project 2022
//

#pragma once
#include <stdint.h>
#include "MFDigInMux.h"

namespace DigInMux
{
    bool    setupArray(uint16_t count);
    void    Add(uint8_t dataPin, uint8_t nRegs, char const *name = "DigInMux");
    void    Clear();
    void    read();
    uint8_t readPin(uint8_t mux, uint8_t pin);
    void    OnTrigger();
}

// DigInMux.h