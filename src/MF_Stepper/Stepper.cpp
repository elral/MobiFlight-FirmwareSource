//
// Stepper.cpp
//
// (C) MobiFlight Project 2022
//

#include "mobiflight.h"
#include "MFStepper.h"
#include "Stepper.h"
#if defined(ARDUINO_ARCH_RP2040) && defined(STEPPER_ON_2ND_CORE)
#include <FreeRTOS.h>
#endif

namespace Stepper
{
    MFStepper *steppers;
    uint8_t    steppersRegistered = 0;
    uint8_t    maxSteppers        = 0;
#if defined(STEPPER_ON_2ND_CORE)
    enum {
        FUNC_MOVETO = 1,
        FUNC_ZETZERO,
        FUNC_SPEEDACCEL,
        FUNC_RESET
    };
#if defined(ARDUINO_ARCH_ESP32)
    // For communication with 2nd core of ESP32
    uint8_t command2ndCore     = 0;
    uint8_t stepper2ndCore     = 0;
    long    Parameter1_2ndCore = 0;
    long    Parameter2_2ndCore = 0;
#endif
#endif

    bool setupArray(uint16_t count)
    {
        if (!FitInMemory(sizeof(MFStepper) * count))
            return false;
        steppers    = new (allocateMemory(sizeof(MFStepper) * count)) MFStepper;
        maxSteppers = count;
        return true;
    }

    void Add(uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4, uint8_t btnPin1, uint8_t mode, int8_t backlash, bool deactivateOutput)
    {
        if (steppersRegistered == maxSteppers)
            return;
        steppers[steppersRegistered] = MFStepper();
        steppers[steppersRegistered].attach(pin1, pin2, pin3, pin4, btnPin1, mode, backlash, deactivateOutput);

        if (btnPin1 > 0) {
            // this triggers the auto reset if we need to reset
            steppers[steppersRegistered].reset();
        }

        // all set
        steppersRegistered++;

#ifdef DEBUG2CMDMESSENGER
        cmdMessenger.sendCmd(kDebug, F("Added stepper"));
#endif
    }

    void Clear()
    {
        for (uint8_t i = 0; i < steppersRegistered; i++) {
            steppers[i].detach();
        }
        steppersRegistered = 0;
#ifdef DEBUG2CMDMESSENGER
        cmdMessenger.sendCmd(kDebug, F("Cleared steppers"));
#endif
    }

    void OnSet()
    {
        uint8_t stepper = (uint8_t)cmdMessenger.readInt16Arg();
        long    newPos  = cmdMessenger.readInt32Arg();

        if (stepper >= steppersRegistered)
            return;
#if defined(ARDUINO_ARCH_RP2040) && defined(STEPPER_ON_2ND_CORE)
        // wait for 2nd core
        rp2040.fifo.pop();
        rp2040.fifo.push(FUNC_MOVETO);
        rp2040.fifo.push(stepper);
        rp2040.fifo.push(newPos);
        rp2040.fifo.push(false);
#elif defined(ARDUINO_ARCH_ESP32) && defined(STEPPER_ON_2ND_CORE)
        // Wait for 2nd core
        while (command2ndCore) {}
        stepper2ndCore     = stepper;
        Parameter1_2ndCore = newPos;
        command2ndCore     = FUNC_MOVETO;
#else
        steppers[stepper].moveTo(newPos);
#endif
    }

    void OnReset()
    {
        uint8_t stepper = (uint8_t)cmdMessenger.readInt16Arg();

        if (stepper >= steppersRegistered)
            return;

#if defined(ARDUINO_ARCH_RP2040) && defined(STEPPER_ON_2ND_CORE)
        // wait for 2nd core
        rp2040.fifo.pop();
        rp2040.fifo.push(FUNC_RESET);
        rp2040.fifo.push(stepper);
        rp2040.fifo.push(false);
        rp2040.fifo.push(false);
#elif defined(ARDUINO_ARCH_ESP32) && defined(STEPPER_ON_2ND_CORE)
        // Wait for 2nd core
        while (command2ndCore) {}
        stepper2ndCore = stepper;
        command2ndCore = FUNC_RESET;
#else
        steppers[stepper].reset();
#endif
    }

    void OnSetZero()
    {
        uint8_t stepper = (uint8_t)cmdMessenger.readInt16Arg();

        if (stepper >= steppersRegistered)
            return;
#if defined(ARDUINO_ARCH_RP2040) && defined(STEPPER_ON_2ND_CORE)
        // wait for 2nd core
        rp2040.fifo.pop();
        rp2040.fifo.push(FUNC_ZETZERO);
        rp2040.fifo.push(stepper);
        rp2040.fifo.push(false);
        rp2040.fifo.push(false);
#elif defined(ARDUINO_ARCH_ESP32) && defined(STEPPER_ON_2ND_CORE)
        // Wait for 2nd core
        while (command2ndCore) {}
        stepper2ndCore = stepper;
        command2ndCore = FUNC_ZETZERO;
#else
        steppers[stepper].setZero();
#endif
    }

    void OnSetSpeedAccel()
    {
        uint8_t  stepper  = (uint8_t)cmdMessenger.readInt16Arg();
        uint16_t maxSpeed = cmdMessenger.readInt16Arg();
        uint16_t maxAccel = cmdMessenger.readInt16Arg();

        if (stepper >= steppersRegistered)
            return;
#if defined(ARDUINO_ARCH_RP2040) && defined(STEPPER_ON_2ND_CORE)
        rp2040.fifo.pop(); // wait for 2nd core
        rp2040.fifo.push(FUNC_SPEEDACCEL);
        rp2040.fifo.push(stepper);
        rp2040.fifo.push(maxSpeed);
        rp2040.fifo.push(maxAccel);
#elif defined(ARDUINO_ARCH_ESP32) && defined(STEPPER_ON_2ND_CORE)
        // Wait for 2nd core
        while (command2ndCore) {}
        stepper2ndCore     = stepper;
        Parameter1_2ndCore = maxSpeed;
        Parameter2_2ndCore = maxAccel;
        command2ndCore     = FUNC_SPEEDACCEL;
#else
        steppers[stepper].setMaxSpeed(maxSpeed);
        steppers[stepper].setAcceleration(maxAccel);
#endif
    }

    void update()
    {
#if !defined(STEPPER_ON_2ND_CORE)
        for (uint8_t i = 0; i < steppersRegistered; i++) {
            steppers[i].update();
        }
#endif
    }

    void PowerSave(bool state)
    {
        for (uint8_t i = 0; i < steppersRegistered; ++i) {
            steppers[i].powerSavingMode(state);
        }
    }

} // namespace

#if defined(ARDUINO_ARCH_RP2040) && defined(STEPPER_ON_2ND_CORE)
/* **********************************************************************************
    This will run the set() function from the custom device on the 2nd core
    Be aware NOT to use the function calls from the Pico SDK!
    Only use the functions from the used framework from EarlePhilHower
    If you mix them up it will give undefined behaviour and strange effects
    see https://arduino-pico.readthedocs.io/en/latest/multicore.html
********************************************************************************** */
void setup1()
{
    rp2040.fifo.push(true); // inform core 1 to be ready
}

void loop1()
{
    uint8_t command, stepper;
    int32_t param1, param2;

    while (1) {
        for (uint8_t i = 0; i < Stepper::steppersRegistered; ++i) {
            Stepper::steppers[i].update();
            if (rp2040.fifo.available()) {
                command = (uint8_t)rp2040.fifo.pop();
                stepper = (uint8_t)rp2040.fifo.pop();
                param1  = (int32_t)rp2040.fifo.pop();
                param2  = (int32_t)rp2040.fifo.pop();
                if (command == Stepper::FUNC_MOVETO) {
                    Stepper::steppers[stepper].moveTo(param1);
                } else if (command == Stepper::FUNC_ZETZERO) {
                    Stepper::steppers[stepper].setZero();
                } else if (command == Stepper::FUNC_SPEEDACCEL) {
                    Stepper::steppers[stepper].setMaxSpeed(param1);
                    Stepper::steppers[stepper].setAcceleration(param2);
                } else if (command == Stepper::FUNC_RESET) {
                    Stepper::steppers[stepper].reset();
                }
                rp2040.fifo.push(true); // inform core 1 to be ready for next command
            }
        }
    }
}
#endif

#if defined(ARDUINO_ARCH_ESP32) && defined(STEPPER_ON_2ND_CORE)
TaskHandle_t loop1Handle;
void         loop1(void *parameter);

void setup1()
{
    BaseType_t xReturned = xTaskCreatePinnedToCore(
        loop1,        /* Function to implement the task */
        "Stepper",    /* Name of the task */
        10000,        /* Stack size in words */
        NULL,         /* Task input parameter */
        0,            /* Priority of the task 0/1/2 = low/normal/high */
        &loop1Handle, /* Task handle. */
        0             /* Core where the task should run */
    );
}

void loop1(void *parameter)
{
    while (1) {
        for (uint8_t i = 0; i < Stepper::steppersRegistered; ++i) {
            Stepper::steppers[i].update();
        }
        if (Stepper::command2ndCore) {
            if (Stepper::command2ndCore == Stepper::FUNC_MOVETO) {
                Stepper::steppers[Stepper::stepper2ndCore].moveTo(Stepper::Parameter1_2ndCore);
            } else if (Stepper::command2ndCore == Stepper::FUNC_ZETZERO) {
                Stepper::steppers[Stepper::stepper2ndCore].setZero();
            } else if (Stepper::command2ndCore == Stepper::FUNC_SPEEDACCEL) {
                Stepper::steppers[Stepper::stepper2ndCore].setMaxSpeed(Stepper::Parameter1_2ndCore);
                Stepper::steppers[Stepper::stepper2ndCore].setAcceleration(Stepper::Parameter2_2ndCore);
            } else if (Stepper::command2ndCore == Stepper::FUNC_RESET) {
                Stepper::steppers[Stepper::stepper2ndCore].reset();
            }
            Stepper::command2ndCore = 0;
        }
    }
}
#endif

// Stepper.cpp
