//
// mobiflight.cpp
//
// (C) MobiFlight Project 2022
//

#include <Arduino.h>
#include "mobiflight.h"
#include "Button.h"
#include "./MF_Encoder/Encoder.h"     // otherwise Teensy specific Encoder lib is used
#include "MFEEPROM.h"
#include "MFInterrupt.h"
#if MF_ANALOG_SUPPORT == 1
#include "Analog.h"
#endif
#if MF_INPUT_SHIFTER_SUPPORT == 1
#include "InputShifter.h"
#endif
#include "Output.h"
#if MF_SEGMENT_SUPPORT == 1
#include "LedSegment.h"
#endif
#if MF_STEPPER_SUPPORT == 1
#include "Stepper.h"
#endif
#if MF_SERVO_SUPPORT == 1
#include "Servos.h"
#endif
#if MF_LCD_SUPPORT == 1
#include "LCDDisplay.h"
#endif
#if MF_OUTPUT_SHIFTER_SUPPORT == 1
#include "OutputShifter.h"
#endif
#if MF_KEYMATRIX_SUPPORT == 1
#include "Keymatrix.h"
#endif
#if MF_DIGIN_MUX_SUPPORT == 1
#include "DigInMux.h"
#endif
#if MF_CUSTOMDEVICE_SUPPORT == 1
#include "CustomDevice.h"
#endif

#define MF_BUTTON_DEBOUNCE_MS     10 // time between updating the buttons
#define MF_ENCODER_DEBOUNCE_MS    1  // time between encoder updates
#define MF_INSHIFTER_POLL_MS      10 // time between input shift reg updates
#define MF_INMUX_POLL_MS          10 // time between dig input mux updates
#define MF_SERVO_DELAY_MS         5  // time between servo updates
#define MF_ANALOGAVERAGE_DELAY_MS 10 // time between updating the analog average calculation
#define MF_ANALOGREAD_DELAY_MS    50 // time between sending analog values

bool                powerSavingMode   = false;
const unsigned long POWER_SAVING_TIME = 60 * 15; // in seconds

#if MF_MUX_SUPPORT == 1
MFMuxDriver MUX;
#endif

#if MF_KEYMATRIX_SUPPORT == 1
uint8_t *keyMatrixColumnPins;
uint8_t *keyMatrixRowPins;
#endif  

// ==================================================
//   Polling interval counters
// ==================================================

typedef struct {
#ifndef USE_INTERRUPT
    uint32_t Buttons  = 0;
    uint32_t Encoders = 0;
#endif
#if MF_SERVO_SUPPORT == 1
    uint32_t Servos = 0;
#endif
#if MF_ANALOG_SUPPORT == 1
    uint32_t AnalogAverage = 0;
    uint32_t Analog        = 0;
#endif
#if MF_INPUT_SHIFTER_SUPPORT == 1 && !defined(USE_INTERRUPT)
    uint32_t InputShifters = 0;
#endif
#if MF_DIGIN_MUX_SUPPORT == 1 // && !defined(USE_INTERRUPT)
    uint32_t DigInMux = 0;
#endif
#if MF_CUSTOMDEVICE_SUPPORT == 1
    uint32_t CustomDevice = 0;
#endif
#if MF_KEYMATRIX_SUPPORT == 1
    uint32_t KeyMatrix = 0;
#endif
} lastUpdate_t;

lastUpdate_t lastUpdate;

extern MFEEPROM MFeeprom;

void initPollIntervals(void)
{
    // Init Time Gap between Inputs, do not read at the same loop
#ifndef USE_INTERRUPT
    lastUpdate.Buttons  = millis();
    lastUpdate.Encoders = millis();
#endif
#if MF_SERVO_SUPPORT == 1
    lastUpdate.Servos = millis() + 2;
#endif
#if MF_ANALOG_SUPPORT == 1
    lastUpdate.AnalogAverage = millis() + 4;
    lastUpdate.Analog        = millis() + 4;
#endif
#if MF_INPUT_SHIFTER_SUPPORT == 1 && !defined(USE_INTERRUPT)
    lastUpdate.InputShifters = millis() + 6;
#endif
#if MF_DIGIN_MUX_SUPPORT == 1 // && !defined(USE_INTERRUPT)
    lastUpdate.DigInMux = millis() + 8;
#endif
#if MF_KEYMATRIX_SUPPORT == 1
  lastUpdate.KeyMatrix  = millis();
#endif  
}

void timedUpdate(void (*fun)(), uint32_t *last, uint8_t intv)
{
    if (millis() - *last >= intv) {
        fun();
        *last = millis();
    }
}

// ************************************************************
// Power saving
// ************************************************************
void SetPowerSavingMode(bool state)
{
    // disable the lights ;)
    powerSavingMode = state;
    Output::PowerSave(state);
#if MF_SEGMENT_SUPPORT == 1
    LedSegment::PowerSave(state);
#endif
#if MF_STEPPER_SUPPORT == 1
    Stepper::PowerSave(state);
#endif
#if MF_CUSTOMDEVICE_SUPPORT == 1
    CustomDevice::PowerSave(state);
#endif
#if MF_OUTPUT_SHIFTER_SUPPORT == 1
    OutputShifter::PowerSave(state);
#endif
#if MF_LCD_SUPPORT == 1
    LCDDisplay::PowerSave(state);
#endif

#ifdef DEBUG2CMDMESSENGER
    if (state)
        cmdMessenger.sendCmd(kDebug, F("Power saving mode on"));
    else
        cmdMessenger.sendCmd(kDebug, F("Power saving mode off"));
#endif
}

void updatePowerSaving()
{
    if (!powerSavingMode && ((millis() - getLastCommandMillis()) > (POWER_SAVING_TIME * 1000))) {
        // enable power saving
        SetPowerSavingMode(true);
    } else if (powerSavingMode && ((millis() - getLastCommandMillis()) < (POWER_SAVING_TIME * 1000))) {
        // disable power saving
        SetPowerSavingMode(false);
    }
}

// ************************************************************
// Reset Board
// ************************************************************
void ResetBoard()
{
    restoreName();
    loadConfig();
}

// ************************************************************
// Setup
// ************************************************************
void setup()
{
    Serial.begin(115200);
    MFeeprom.init();
    attachCommandCallbacks();
    cmdMessenger.printLfCr();
    ResetBoard();
    initPollIntervals();
/*
uint8_t ColumnPins = 8;
uint8_t RowPins = 8;
keyMatrixColumnPins = new (allocateMemory(ColumnPins)) uint8_t;
    keyMatrixColumnPins[0] = 0;
    keyMatrixColumnPins[1] = 1;
    keyMatrixColumnPins[2] = 2;
    keyMatrixColumnPins[3] = 3;
    keyMatrixColumnPins[4] = 4;
    keyMatrixColumnPins[5] = 5;
    keyMatrixColumnPins[6] = 6;
    keyMatrixColumnPins[7] = 7;
keyMatrixRowPins = new (allocateMemory(RowPins)) uint8_t;
    keyMatrixRowPins[0] = 8;
    keyMatrixRowPins[1] = 9;
    keyMatrixRowPins[2] = 10;
    keyMatrixRowPins[3] = 11;
    keyMatrixRowPins[4] = 12;
    keyMatrixRowPins[5] = 13;
    keyMatrixRowPins[6] = 14;
    keyMatrixRowPins[7] = 15;
    Keymatrix::Add(8, keyMatrixColumnPins, 8, keyMatrixRowPins, "Keymatrix");
*/
#ifdef USE_INTERRUPT
    setup_interrupt();
#endif
}

// ************************************************************
// Loop function
// ************************************************************
void loop()
{
    // Process incoming serial data, and perform callbacks
    cmdMessenger.feedinSerialData();
    updatePowerSaving();

    // if config has been reset and still is not activated
    // do not perform updates
    // to prevent mangling input for config (shared buffers)
    if (getStatusConfig()) {
#ifndef USE_INTERRUPT
        timedUpdate(Button::poll, &lastUpdate.Buttons, MF_BUTTON_DEBOUNCE_MS);
        timedUpdate(Encoder::poll, &lastUpdate.Encoders, MF_ENCODER_DEBOUNCE_MS);
    #if MF_INPUT_SHIFTER_SUPPORT == 1
        timedUpdate(InputShifter::poll, &lastUpdate.InputShifters, MF_INSHIFTER_POLL_MS);
    #endif
    #if MF_DIGIN_MUX_SUPPORT == 1
        // timedUpdate(DigInMux::poll, &lastUpdate.DigInMux, MF_INMUX_POLL_MS);
        // uncomment this once read() is splitted into read() and poll(), see below
    #endif
#endif
        Button::read();
        Encoder::read();
#if MF_ANALOG_SUPPORT == 1
        timedUpdate(Analog::readAverage, &lastUpdate.AnalogAverage, MF_ANALOGAVERAGE_DELAY_MS);
        timedUpdate(Analog::read, &lastUpdate.Analog, MF_ANALOGREAD_DELAY_MS);
        //Analog::read();     // unless AnalogAverage() is not called, no new value is available -> new values not faster than 50ms
#endif
#if MF_INPUT_SHIFTER_SUPPORT == 1
        InputShifter::read();
#endif
#if MF_DIGIN_MUX_SUPPORT == 1
        timedUpdate(DigInMux::read, &lastUpdate.DigInMux, MF_INMUX_POLL_MS);
        // DigInMux::read();
        // split into read() and poll(), poll() needs to be called every 10ms
        // then uncomment read() here and poll() in the upper part (#ifndef USE_INTERRUPT) and uncomment poll() in ISR
#endif

#if MF_STEPPER_SUPPORT == 1
        Stepper::update();
#endif

#if MF_SERVO_SUPPORT == 1
        // Servo smoothing depends on the time between calling update(), so it must be done every 5ms
        timedUpdate(Servos::update, &lastUpdate.Servos, MF_SERVO_DELAY_MS);
#endif

#if MF_KEYMATRIX_SUPPORT == 1
    timedUpdate(Keymatrix::read, &lastUpdate.KeyMatrix, MF_BUTTON_DEBOUNCE_MS);
#endif

#if MF_CUSTOMDEVICE_SUPPORT == 1 && defined(MF_CUSTOMDEVICE_HAS_UPDATE)
#ifdef MF_CUSTOMDEVICE_POLL_MS
        timedUpdate(CustomDevice::update, &lastUpdate.CustomDevice, MF_CUSTOMDEVICE_POLL_MS);
#else
        CustomDevice::update();
#endif
#endif

        // lcds, outputs, outputshifters, segments do not need update
    }
}

// mobiflight.cpp
