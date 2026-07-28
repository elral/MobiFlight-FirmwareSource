//
// MFOutputShifter.cpp
//
// (C) MobiFlight Project 2022
//

#include "MFOutputShifter.h"
#include "MFShiftData.h"
#include "allocateMem.h"

MFOutputShifter::MFOutputShifter()
{
    _initialized = false;
}

void MFOutputShifter::setPins(uint8_t *pins, uint8_t value)
{
    if (!_initialized) return;

    for (uint8_t i = 0; i < _moduleCount; i++) {
        if (value != MF_LOW)
            _lastState[i] |= pins[i];
        else
            _lastState[i] &= ~pins[i];
    }
    update();
}

bool MFOutputShifter::attach(uint8_t latchPin, uint8_t clockPin, uint8_t dataPin, uint8_t moduleCount)
{
    _initialized  = true;
#ifdef USE_FAST_IO
    _latchPin.Port = portOutputRegister(digitalPinToPort(latchPin));
    _latchPin.Mask = digitalPinToBitMask(latchPin);
    _clockPin.Port = portOutputRegister(digitalPinToPort(clockPin));
    _clockPin.Mask = digitalPinToBitMask(clockPin);
    _dataPin.Port  = portInputRegister(digitalPinToPort(dataPin));
    _dataPin.Mask  = digitalPinToBitMask(dataPin);
#else
    _latchPin    = latchPin;
    _clockPin    = clockPin;
    _dataPin     = dataPin;
#endif
    _moduleCount  = moduleCount;

    pinMode(latchPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, OUTPUT);

    _lastState = static_cast<uint8_t*>(MF_ALLOC_BYTES(_moduleCount));
    if (!_lastState) return false;

    clear();
    return true;
}

void MFOutputShifter::detach()
{
    _initialized = false;
}

void MFOutputShifter::clear()
{
    for (uint8_t i = 0; i < _moduleCount; i++) {
        _lastState[i] = 0xFF * MF_LOW;
    }
    update();
}

void MFOutputShifter::update()
{
    DIGITALWRITE(_latchPin, LOW);
    for (uint8_t i = _moduleCount; i > 0; i--) {
        for (int8_t j = 7; j >= 0; j--) {
            DIGITALWRITE(_dataPin, (_lastState[i - 1] & (1 << (j))));
            DIGITALWRITE(_clockPin, HIGH);
            DIGITALWRITE(_clockPin, LOW);
        }
    }
    DIGITALWRITE(_latchPin, HIGH);
}

void MFOutputShifter::powerSavingMode(bool state)
{
    if (state) {
        DIGITALWRITE(_latchPin, LOW);
        for (uint8_t i = _moduleCount; i > 0; i--) {
            DIGITALWRITE(_dataPin, 0xFF * MF_LOW);
            DIGITALWRITE(_clockPin, HIGH);
            DIGITALWRITE(_clockPin, LOW);
        }
        DIGITALWRITE(_latchPin, HIGH);
    } else {
        update();
    }
}

// MFOutputShifter.cpp
