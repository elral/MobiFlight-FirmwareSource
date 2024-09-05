//
// commandmessenger.cpp
//
// (C) MobiFlight Project 2022
//

#include "mobiflight.h"

#include "Button.h"
#include "Encoder.h"
#if MF_ANALOG_SUPPORT == 1
#include "AnalogIn.h"
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
#if MF_DIGIN_MUX_SUPPORT == 1
#include "DigInMux.h"
#endif
#if MF_CUSTOMDEVICE_SUPPORT == 1
#include "CustomDevice.h"
#endif

CmdMessenger  cmdMessenger = CmdMessenger(Serial);
unsigned long lastCommand;

void OnSetPowerSavingMode();
void OnTrigger();
void OnUnknownCommand();
void JumpToBootloader();

// Callbacks define on which received commands we take action
void attachCommandCallbacks()
{
    // Attach callback methods
    cmdMessenger.attach(OnUnknownCommand);
    cmdMessenger.attach(kJumpToBootloader, JumpToBootloader);

#if MF_SEGMENT_SUPPORT == 1
    cmdMessenger.attach(kInitModule, LedSegment::OnInitModule);
    cmdMessenger.attach(kSetModule, LedSegment::OnSetModule);
    cmdMessenger.attach(kSetModuleBrightness, LedSegment::OnSetModuleBrightness);
    cmdMessenger.attach(kSetModuleSingleSegment, LedSegment::OnSetModuleSingleSegment);
#endif

    cmdMessenger.attach(kSetPin, Output::OnSet);

#if MF_STEPPER_SUPPORT == 1
    cmdMessenger.attach(kSetStepper, Stepper::OnSet);
    cmdMessenger.attach(kResetStepper, Stepper::OnReset);
    cmdMessenger.attach(kSetZeroStepper, Stepper::OnSetZero);
    cmdMessenger.attach(kSetStepperSpeedAccel, Stepper::OnSetSpeedAccel);
#endif

#if MF_SERVO_SUPPORT == 1
    cmdMessenger.attach(kSetServo, Servos::OnSet);
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

#if MF_LCD_SUPPORT == 1
    cmdMessenger.attach(kSetLcdDisplayI2C, LCDDisplay::OnSet);
#endif

#if MF_OUTPUT_SHIFTER_SUPPORT == 1
    cmdMessenger.attach(kSetShiftRegisterPins, OutputShifter::OnSet);
#endif

#if MF_CUSTOMDEVICE_SUPPORT == 1
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
#if MF_INPUT_SHIFTER_SUPPORT == 1
    InputShifter::OnTrigger();
#endif
#if MF_DIGIN_MUX_SUPPORT == 1
    DigInMux::OnTrigger();
#endif
#if MF_ANALOG_SUPPORT == 1
    Analog::OnTrigger();
#endif
}

void JumpToBootloader(void)
{
#if defined(ARDUINO_NUCLEO_L496ZG)
     HAL_SuspendTick();

     /* Clear Interrupt Enable Register & Interrupt Pending Register */
     for (int i=0;i<5;i++)
     {
         NVIC->ICER[i]=0xFFFFFFFF;
         NVIC->ICPR[i]=0xFFFFFFFF;
     }

     HAL_FLASH_Unlock();

     HAL_FLASH_OB_Unlock();

     // RM0351 Rev 7 Page 93/1903
     // AN2606 Rev 44 Page 23/372
     CLEAR_BIT(FLASH->OPTR, FLASH_OPTR_nBOOT0);
     SET_BIT(FLASH->OPTR, FLASH_OPTR_nBOOT1);
     CLEAR_BIT(FLASH->OPTR, FLASH_OPTR_nSWBOOT0);

     SET_BIT(FLASH->CR, FLASH_CR_OPTSTRT);

     while(READ_BIT(FLASH->SR, FLASH_SR_BSY));

     HAL_FLASH_OB_Launch();
#endif
}

// commandmessenger.cpp
