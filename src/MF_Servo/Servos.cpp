//
// Servos.cpp
//
// (C) MobiFlight Project 2022
//

#include "commandmessenger.h"
#include "allocateMem.h"
#include "MFServo.h"
#include "Servos.h"

namespace Servos
{
    MFServo *servos;
    uint8_t  servosRegistered = 0;
    uint8_t  maxServos        = 0;

    bool setupArray(uint16_t count)
    {
        if (!FitInMemory(sizeof(MFServo) * count))
            return false;
        servos    = new (allocateMemory(sizeof(MFServo) * count)) MFServo;
        maxServos = count;
        return true;
    }

    uint8_t Add(uint8_t pin)
    {
        if (servosRegistered == maxServos)
            return 0xFF;
        servos[servosRegistered] = MFServo();
        servos[servosRegistered].attach(pin, true);
        servosRegistered++;
#ifdef DEBUG2CMDMESSENGER
        cmdMessenger.sendCmd(kDebug, F("Added servos"));
#endif
        return servosRegistered - 1;
    }

    void Clear()
    {
        for (uint8_t i = 0; i < servosRegistered; i++) {
            servos[i].detach();
        }
        servosRegistered = 0;
#ifdef DEBUG2CMDMESSENGER
        cmdMessenger.sendCmd(kDebug, F("Cleared servos"));
#endif
    }

    void OnSet()
    {
        int servo    = cmdMessenger.readInt16Arg();
        int newValue = cmdMessenger.readInt16Arg();
        if (servo >= servosRegistered)
            return;
        servos[servo].moveTo(newValue);
    }

    int16_t getActualValue(uint8_t channel)
    {
        return servos[channel].getActualValue(); // range is 0 ... 1024
    }

    void update()
    {
        for (uint8_t i = 0; i < servosRegistered; i++) {
            servos[i].update();
        }
    }
} // namespace

// Servos.cpp
