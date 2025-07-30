//
// Output.cpp
//
// (C) MobiFlight Project 2022
//

#include "commandmessenger.h"
#include "allocateMem.h"
#include "MFOutput.h"
#include "Output.h"

namespace Output
{
    MFOutput *outputs;
    uint8_t   outputsRegistered = 0;
    uint8_t   maxOutputs        = 0;

    bool setupArray(uint16_t count)
    {
        if (!FitInMemory(sizeof(MFOutput) * count))
            return false;
        outputs    = new (allocateMemory(sizeof(MFOutput) * count)) MFOutput;
        maxOutputs = count;
        return true;
    }

    uint8_t Add(uint8_t pin)
    {
        if (outputsRegistered == maxOutputs)
            return 0xFF;
        outputs[outputsRegistered] = MFOutput();
        outputs[outputsRegistered].attach(pin);
        outputsRegistered++;
#ifdef DEBUG2CMDMESSENGER
        cmdMessenger.sendCmd(kDebug, F("Added output"));
#endif
        return outputsRegistered - 1;
    }

    void Clear()
    {
        outputsRegistered = 0;
#ifdef DEBUG2CMDMESSENGER
        cmdMessenger.sendCmd(kDebug, F("Cleared outputs"));
#endif
    }

    void OnSet()
    {
        // Read led state argument, interpret string as boolean
        int output   = cmdMessenger.readInt16Arg();
        int state = cmdMessenger.readInt16Arg();

        outputs[output].set(state);
    }

    void PowerSave(bool state)
    {
        for (uint8_t i = 0; i < outputsRegistered; ++i) {
            outputs[i].powerSavingMode(state);
        }
    }
} // namespace

// Output.cpp
