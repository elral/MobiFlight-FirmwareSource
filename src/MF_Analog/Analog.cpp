//
// Analog.cpp
//
// (C) MobiFlight Project 2022
//

#include "mobiflight.h"
#include "MFAnalog.h"
#include "Analog.h"

#if MF_ANALOG_SUPPORT == 1
namespace Analog
{
    MFAnalog *analog;
    uint8_t   analogRegistered = 0;
    uint8_t   maxAnalogIn      = 0;

    void handlerOnAnalogChange(int value, const char *name)
    {
        if (!getBoardReady())
            return;
        cmdMessenger.sendCmdStart(kAnalogChange);
        cmdMessenger.sendCmdArg(name);
        cmdMessenger.sendCmdArg(value);
        cmdMessenger.sendCmdEnd();
    };

    bool setupArray(uint16_t count)
    {
        if (!FitInMemory(sizeof(MFAnalog) * count))
            return false;
        analog      = new (allocateMemory(sizeof(MFAnalog) * count)) MFAnalog;
        maxAnalogIn = count;
        return true;
    }

    uint8_t Add(uint8_t pin, char const *name, uint8_t sensitivity, bool deprecated)
    {
        if (analogRegistered == maxAnalogIn)
            return 0xFF;

        analog[analogRegistered] = MFAnalog();
        analog[analogRegistered].attach(pin, name, sensitivity, deprecated);
        MFAnalog::attachHandler(handlerOnAnalogChange);
        analogRegistered++;
#ifdef DEBUG2CMDMESSENGER
        cmdMessenger.sendCmd(kDebug, F("Added analog device "));
#endif
        return analogRegistered - 1;
    }

    void Clear(void)
    {
        analogRegistered = 0;
#ifdef DEBUG2CMDMESSENGER
        cmdMessenger.sendCmd(kDebug, F("Cleared analog devices"));
#endif
    }

    void read(void)
    {
        for (uint8_t i = 0; i < analogRegistered; i++) {
            analog[i].update();
        }
    }

    int16_t getActualValue(uint8_t channel)
    {
        return analog[channel].getActualValue(); // range is 0 ... 1024
    }

    void readAverage(void)
    {
        for (uint8_t i = 0; i < analogRegistered; i++) {
            analog[i].readBuffer();
        }
    }

    void OnTrigger()
    {
        // Scan and transit current values
        for (uint8_t i = 0; i < analogRegistered; i++) {
            analog[i].retrigger();
        }
    }

} // namespace Analog
#endif

// Analog.cpp
