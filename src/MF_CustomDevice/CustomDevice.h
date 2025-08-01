//
// CustomDevice.h
//
// (C) MobiFlight Project 2022
//

#pragma once

#include <Arduino.h>

namespace CustomDevice
{
#define START_STOP_2ND_CORE -1

    bool setupArray(uint16_t count);
    void Add(uint16_t adrPin, uint16_t adrType, uint16_t adrConfig, bool configFromFlash);
    void Clear();
    void update();
    void OnSet();
    void PowerSave(bool state);
#if defined(USE_2ND_CORE) && defined(ARDUINO_ARCH_RP2040)
    void stopUpdate2ndCore(bool stop);
#endif
}