//
// commandmessenger.cpp
//
// (C) MobiFlight Project 2022
//

#include "commandmessenger.h"
#include "config.h"
#include "Button.h"
#include "Encoder.h"
#ifdef MF_ANALOG_SUPPORT
#include "Analog.h"
#endif
#ifdef MF_INPUT_SHIFTER_SUPPORT
#include "InputShifter.h"
#endif
#include "Output.h"
#ifdef MF_SEGMENT_SUPPORT
#include "LedSegment.h"
#endif
#ifdef MF_STEPPER_SUPPORT
#include "Stepper.h"
#endif
#ifdef MF_SERVO_SUPPORT
#include "Servos.h"
#endif
#ifdef MF_LCD_SUPPORT
#include "LCDDisplay.h"
#endif
#ifdef MF_OUTPUT_SHIFTER_SUPPORT
#include "OutputShifter.h"
#endif
#ifdef MF_DIGIN_MUX_SUPPORT
#include "DigInMux.h"
#endif
#ifdef MF_CUSTOMDEVICE_SUPPORT
#include "CustomDevice.h"
#endif

CmdMessenger  cmdMessenger = CmdMessenger(Serial);
unsigned long lastCommand;

void OnSetPowerSavingMode();
void OnTrigger();
void OnUnknownCommand();

void trackCommand()
{
    lastCommand = millis();
}

void PowerSavingWrapper_Output_OnSet()
{
    trackCommand();
    Output::OnSet();
}

#ifdef MF_SEGMENT_SUPPORT
void PowerSavingWrapper_LedSegmentModule_OnSet()
{
    trackCommand();
    LedSegment::OnSetModule();
}
void PowerSavingWrapper_LedSegmentSingleSegment_OnSet()
{
    trackCommand();
    LedSegment::OnSetModuleSingleSegment();
}
#endif

#ifdef MF_STEPPER_SUPPORT
void PowerSavingWrapper_Stepper_OnSet()
{
    trackCommand();
    Stepper::OnSet();
}
#endif

#ifdef MF_LCD_SUPPORT
void PowerSavingWrapper_LCD_OnSet()
{
    trackCommand();
    LCDDisplay::OnSet();
}
#endif

#ifdef MF_OUTPUT_SHIFTER_SUPPORT
void PowerSavingWrapper_OutputShifter_OnSet()
{
    trackCommand();
    OutputShifter::OnSet();
}
#endif

#ifdef MF_SERVO_SUPPORT
void PowerSavingWrapper_Servo_OnSet()
{
    trackCommand();
    Servos::OnSet();
}
#endif

#ifdef MF_CUSTOMDEVICE_SUPPORT
void PowerSavingWrapper_CustomDevice_OnSet()
{
    trackCommand();
    CustomDevice::OnSet();
}
#endif


// Callbacks define on which received commands we take action
void attachCommandCallbacks()
{
    // Attach callback methods
    cmdMessenger.attach(OnUnknownCommand);

#ifdef MF_SEGMENT_SUPPORT
    cmdMessenger.attach(kInitModule, LedSegment::OnInitModule);
    cmdMessenger.attach(kSetModule, PowerSavingWrapper_LedSegmentModule_OnSet);
    cmdMessenger.attach(kSetModuleBrightness, LedSegment::OnSetModuleBrightness);
    cmdMessenger.attach(kSetModuleSingleSegment, PowerSavingWrapper_LedSegmentSingleSegment_OnSet);
#endif

    cmdMessenger.attach(kSetPin, Output::OnSet);

#ifdef MF_STEPPER_SUPPORT
    cmdMessenger.attach(kSetStepper, PowerSavingWrapper_Stepper_OnSet);
    cmdMessenger.attach(kResetStepper, Stepper::OnReset);
    cmdMessenger.attach(kSetZeroStepper, Stepper::OnSetZero);
    cmdMessenger.attach(kSetStepperSpeedAccel, Stepper::OnSetSpeedAccel);
#endif

#ifdef MF_SERVO_SUPPORT
    cmdMessenger.attach(kSetServo, PowerSavingWrapper_Servo_OnSet);
#endif

    cmdMessenger.attach(kGetInfo, OnGetInfo);
    cmdMessenger.attach(kGetConfig, OnGetConfig);
    cmdMessenger.attach(kSetConfig, OnSetConfig);
    cmdMessenger.attach(kResetConfig, OnResetConfig);
    cmdMessenger.attach(kSaveConfig, OnSaveConfig);
    cmdMessenger.attach(kActivateConfig, OnActivateConfig);
    cmdMessenger.attach(kSetName, OnSetName);
    cmdMessenger.attach(kGenNewSerial, OnGenNewSerial);
    cmdMessenger.attach(kTrigger, OnTrigger);
    cmdMessenger.attach(kSetPowerSavingMode, OnSetPowerSavingMode);

#ifdef MF_LCD_SUPPORT
    cmdMessenger.attach(kSetLcdDisplayI2C, PowerSavingWrapper_LCD_OnSet);
#endif

#ifdef MF_OUTPUT_SHIFTER_SUPPORT
    cmdMessenger.attach(kSetShiftRegisterPins, PowerSavingWrapper_OutputShifter_OnSet);
#endif

#ifdef MF_CUSTOMDEVICE_SUPPORT
    cmdMessenger.attach(kSetCustomDevice, CustomDevice::OnSet);
#endif

#ifdef DEBUG2CMDMESSENGER
    cmdMessenger.sendCmd(kDebug, F("Attached callbacks"));
#endif
}

// Called when a received command has no attached function
void OnUnknownCommand()
{
    lastCommand = millis();
    cmdMessenger.sendCmd(kStatus, F("n/a"));
}

// Handles requests from the desktop app to disable power saving mode
void OnSetPowerSavingMode()
{
    bool enablePowerSavingMode = cmdMessenger.readBoolArg();

    // If the request is to enable powersaving mode then set the last command time
    // to the earliest possible time. The next time loop() is called in mobiflight.cpp
    // this will cause power saving mode to get turned on.
    if (enablePowerSavingMode) {
#ifdef DEBUG2CMDMESSENGER
        cmdMessenger.sendCmd(kDebug, F("Enabling power saving mode via request"));
#endif
        lastCommand = 0;
    }
    // If the request is to disable power saving mode then simply set the last command
    // to now. The next time loop() is called in mobiflight.cpp this will cause
    // power saving mode to get turned off.
    else {
#ifdef DEBUG2CMDMESSENGER
        cmdMessenger.sendCmd(kDebug, F("Disabling power saving mode via request"));
#endif
        lastCommand = millis();
    }
}

uint32_t getLastCommandMillis()
{
    return lastCommand;
}

void OnTrigger()
{
    Button::OnTrigger();
#ifdef MF_INPUT_SHIFTER_SUPPORT
    InputShifter::OnTrigger();
#endif
#ifdef MF_DIGIN_MUX_SUPPORT
    DigInMux::OnTrigger();
#endif
#ifdef MF_ANALOG_SUPPORT
    Analog::OnTrigger();
#endif
}

// commandmessenger.cpp
