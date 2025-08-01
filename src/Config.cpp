//
// config.cpp
//
// (C) MobiFlight Project 2022
//

#include "config.h"
#include "commandmessenger.h"
#include "allocateMem.h"
#include "MFEEPROM.h"
#include "Button.h"
#include "Encoder.h"
#include "Output.h"
#if !defined(ARDUINO_ARCH_AVR)
#include "ArduinoUniqueID.h"
#endif

#ifdef MF_ANALOG_SUPPORT
#include "Analog.h"
#endif
#ifdef MF_INPUT_SHIFTER_SUPPORT
#include "InputShifter.h"
#endif
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
#ifdef MF_MUX_SUPPORT
#include "MFMuxDriver.h"
#endif
#ifdef MF_DIGIN_MUX_SUPPORT
#include "DigInMux.h"
#endif
#ifdef MF_CUSTOMDEVICE_SUPPORT
#include "CustomDevice.h"
#endif
#ifdef HAS_CONFIG_IN_FLASH
#include "MFCustomDevicesConfig.h"
#else
const char CustomDeviceConfig[] PROGMEM = {};
#endif

// The build version comes from an environment variable
#define STRINGIZER(arg) #arg
#define STR_VALUE(arg)  STRINGIZER(arg)
#define VERSION         STR_VALUE(BUILD_VERSION)
#define CORE_VERSION    STR_VALUE(CORE_BUILD_VERSION)

MFEEPROM MFeeprom;

#if MF_MUX_SUPPORT == 1
extern MFMuxDriver MUX;
#endif

const uint8_t MEM_OFFSET_NAME   = 0;
const uint8_t MEM_LEN_NAME      = 48;
const uint8_t MEM_OFFSET_SERIAL = MEM_OFFSET_NAME + MEM_LEN_NAME;
const uint8_t MEM_LEN_SERIAL    = 11;
const uint8_t MEM_OFFSET_CONFIG = MEM_OFFSET_NAME + MEM_LEN_NAME + MEM_LEN_SERIAL;

#ifdef ARDUINO_ARCH_AVR
char serial[11]; // 3 characters for "SN-",7 characters for "xyz-zyx" plus terminating NULL
#else
char serial[3 + UniqueIDsize * 2 + 1]; // 3 characters for "SN-", UniqueID as HEX String, terminating NULL
#endif
char           name[MEM_LEN_NAME]              = MOBIFLIGHT_NAME;
const int      MEM_LEN_CONFIG                  = MEMLEN_CONFIG;
char           nameBuffer[MEMLEN_NAMES_BUFFER] = "";
uint16_t       configLengthEEPROM              = 0;
boolean        configActivated                 = false;
uint16_t       pNameBuffer                     = 0; // pointer for nameBuffer during reading of config
const uint16_t configLengthFlash               = sizeof(CustomDeviceConfig);
bool boardReady                                = false;

void resetConfig();
void readConfig();
void _activateConfig();
void readConfigFromMemory(bool configFromFlash);

bool configStoredInFlash()
{
    return configLengthFlash > 0;
}
bool configStoredInEEPROM()
{
    return configLengthEEPROM > 0;
}

bool getBoardReady()
{
    return boardReady;
}

// ************************************************************
// configBuffer handling
// ************************************************************
// reads the EEPROM until NUL terminator and returns the number of characters incl. terminator, starting from given address
bool readconfigLengthEEPROM()
{
    uint16_t addreeprom = MEM_OFFSET_CONFIG;
    uint16_t length     = MFeeprom.get_length();
    configLengthEEPROM  = 0;

    if (MFeeprom.read_byte(MEM_OFFSET_CONFIG) == 0xFF)
        return false;
    while (MFeeprom.read_byte(addreeprom++) != 0x00) {
        configLengthEEPROM++;
        if (addreeprom > length) {
            cmdMessenger.sendCmd(kStatus, F("Loading config failed")); // text or "-1" like config upload?
            return false;
        }
    }
    return true;
}

void loadConfig()
{
#ifdef DEBUG2CMDMESSENGER
    cmdMessenger.sendCmd(kDebug, F("Load config"));
#endif
    readconfigLengthEEPROM();
    readConfig();
    _activateConfig();
}

void OnSetConfig()
{
#ifdef DEBUG2CMDMESSENGER
    cmdMessenger.sendCmd(kDebug, F("Setting config start"));
#endif
    // A config can be in flash or in EEPROM, but only one option must be used
    // Once a config is in EEPROM, this config will be loaded and reported to the connector
    // If no config is in EEPROM, the config from flash will be used if available
    // This ensures backwards compatibility if a board gets updated with a config in flash
    // but also have a user config in EEPROM
    char   *cfg    = cmdMessenger.readStringArg();
    uint8_t cfgLen = strlen(cfg);

    if (!configStoredInFlash()) {
        bool maxConfigLengthNotExceeded = configLengthEEPROM + cfgLen + 1 < MEM_LEN_CONFIG;
        if (maxConfigLengthNotExceeded) {
            // save the received config string including the terminatung NULL (+1) to EEPROM
            MFeeprom.write_block(MEM_OFFSET_CONFIG + configLengthEEPROM, cfg, cfgLen + 1);
            configLengthEEPROM += cfgLen;
            cmdMessenger.sendCmd(kStatus, configLengthEEPROM);
        } else {
            // staus message to connector, failure on setting config
            // connector does not check for status = -1
            cmdMessenger.sendCmd(kStatus, -1);
        }
#ifdef DEBUG2CMDMESSENGER
        cmdMessenger.sendCmd(kDebug, F("Setting config end"));
#endif
    } else {
        // connector does not check for status = -1
        cmdMessenger.sendCmd(kStatus, -1);
    }
}

void resetConfig()
{
    Button::Clear();
    Encoder::Clear();
    Output::Clear();
#ifdef MF_SEGMENT_SUPPORT
    LedSegment::Clear();
#endif
#ifdef MF_SERVO_SUPPORT
    Servos::Clear();
#endif
#ifdef MF_STEPPER_SUPPORT
#if defined(STEPPER_ON_2ND_CORE) && defined(ARDUINO_ARCH_RP2040)
    Stepper::stopUpdate2ndCore(true);
#endif
    Stepper::Clear();
#if defined(STEPPER_ON_2ND_CORE) && defined(ARDUINO_ARCH_RP2040)
    Stepper::stopUpdate2ndCore(false);
#endif
#endif
#ifdef MF_LCD_SUPPORT
    LCDDisplay::Clear();
#endif
#ifdef MF_ANALOG_SUPPORT
    Analog::Clear();
#endif
#ifdef MF_OUTPUT_SHIFTER_SUPPORT
    OutputShifter::Clear();
#endif
#ifdef MF_INPUT_SHIFTER_SUPPORT
    InputShifter::Clear();
#endif
#ifdef MF_DIGIN_MUX_SUPPORT
    DigInMux::Clear();
#endif
#ifdef MF_CUSTOMDEVICE_SUPPORT
#if defined(USE_2ND_CORE) && defined(ARDUINO_ARCH_RP2040)
    CustomDevice::stopUpdate2ndCore(true);
#endif
    CustomDevice::Clear();
#if defined(USE_2ND_CORE) && defined(ARDUINO_ARCH_RP2040)
    CustomDevice::stopUpdate2ndCore(false);
#endif
#endif
    configLengthEEPROM = 0;
    configActivated    = false;
    pNameBuffer        = 0;
    ClearMemory();
}

void OnResetConfig()
{
    resetConfig();
    cmdMessenger.sendCmd(kStatus, F("OK"));
}

void OnSaveConfig()
{
    MFeeprom.commit();
    cmdMessenger.sendCmd(kConfigSaved, F("OK"));
}

void OnActivateConfig()
{
    readConfig();
    _activateConfig();
}

void _activateConfig()
{
    configActivated = true;
    cmdMessenger.sendCmd(kConfigActivated, F("OK"));
}

// reads an ascii value which is '.' terminated from EEPROM or Flash and returns it's value
uint8_t readUint(volatile uint16_t *addrMem, bool configFromFlash)
{
    char    params[4] = {0}; // max 3 (255) digits NULL terminated
    uint8_t counter   = 0;
    do {
        if (configFromFlash) {
            params[counter++] = pgm_read_byte_near(CustomDeviceConfig + (*addrMem)++);
        } else {
            params[counter++] = MFeeprom.read_byte((*addrMem)++);
        }
    } while (params[counter - 1] != '.' && counter < sizeof(params)); // reads until limiter '.' and for safety reason not more then size of params[]
    params[counter - 1] = 0x00; // replace '.' by NULL to terminate the string
    return atoi(params);
}

// reads a string from EEPROM or Flash at given address which is ':' terminated and saves it in the nameBuffer
// once the nameBuffer is not needed anymore, just read until the ":" termination -> see function below
bool readName(uint16_t *addrMem, char *buffer, uint16_t *pBuffer, bool configFromFlash)
{
    char     temp   = 0;
    uint16_t length = MFeeprom.get_length();
    do {
        if (configFromFlash) {
            temp = pgm_read_byte_near(CustomDeviceConfig + (*addrMem)++); // read the first character
            if (*addrMem > configLengthFlash)                             // abort if config array size will be exceeded
                return false;
        } else {
            temp = MFeeprom.read_byte((*addrMem)++); // read the first character
            if (*addrMem > length)                   // abort if EEPROM size will be exceeded
                return false;
        }
        buffer[(*pBuffer)++] = temp;         // save character and locate next buffer position
        if (*pBuffer >= MEMLEN_NAMES_BUFFER) // nameBuffer will be exceeded
        {
            return false; // abort copying from EEPROM to nameBuffer
        }
    } while (temp != ':'); // reads until limiter ':' and locates the next free buffer position
    buffer[(*pBuffer) - 1] = 0x00; // replace ':' by NULL, terminates the string
    return true;
}

// steps thru the EEPRROM or Flash until the delimiter is detected
// it could be ":" for end of one device config
// or "." for end of type/pin/config entry for custom device
bool readEndCommand(uint16_t *addrMem, uint8_t delimiter, bool configFromFlash)
{
    char     temp   = 0;
    uint16_t length = MFeeprom.get_length();
    do {
        if (configFromFlash) {
            temp = pgm_read_byte_near(CustomDeviceConfig + (*addrMem)++);
            if (*addrMem > configLengthFlash) // abort if config array size will be exceeded
                return false;
        } else {
            temp = MFeeprom.read_byte((*addrMem)++);
            if (*addrMem > length) // abort if EEPROM size will be exceeded
                return false;
        }
    } while (temp != delimiter);
    return true;
}

void sendFailureMessage(const char *deviceName)
{
    cmdMessenger.sendCmdStart(kStatus);
    cmdMessenger.sendCmdArg(deviceName);
    cmdMessenger.sendCmdArg(F("does not fit in Memory"));
    cmdMessenger.sendCmdEnd();
}

bool GetArraySizes(uint8_t *numberDevices, bool configFromFlash)
{
    bool     copy_success = true;
    uint16_t addrMem;
    uint8_t  device;
    if (configFromFlash)
        addrMem = 0;
    else
        addrMem = MEM_OFFSET_CONFIG;

    device = readUint(&addrMem, configFromFlash);

    // step through the Memory and calculate the number of devices for each type
    do {
        numberDevices[device]++;
        copy_success = readEndCommand(&addrMem, ':', configFromFlash); // check EEPROM until end of name
        device       = readUint(&addrMem, configFromFlash);
    } while (device && copy_success);

    if (!copy_success) { // too much/long names for input devices -> tbd how to handle this!!
        cmdMessenger.sendCmd(kStatus, F("Failure, EEPROM size exceeded "));
        return false;
    }
    return true;
}

void InitArrays(uint8_t *numberDevices)
{
    // Call the function to allocate required memory for the arrays of each type
    if (!Button::setupArray(numberDevices[kTypeButton]))
        sendFailureMessage("Button");
    if (!Output::setupArray(numberDevices[kTypeOutput]))
        sendFailureMessage("Output");
#ifdef MF_SEGMENT_SUPPORT
    if (!LedSegment::setupArray(numberDevices[kTypeLedSegmentDeprecated] + numberDevices[kTypeLedSegmentMulti]))
        sendFailureMessage("7Segment");
#endif
#ifdef MF_STEPPER_SUPPORT
    if (!Stepper::setupArray(numberDevices[kTypeStepper] + numberDevices[kTypeStepperDeprecated1] + numberDevices[kTypeStepperDeprecated2]))
        sendFailureMessage("Stepper");
#endif
#ifdef MF_SERVO_SUPPORT
    if (!Servos::setupArray(numberDevices[kTypeServo]))
        sendFailureMessage("Servo");
#endif
    if (!Encoder::setupArray(numberDevices[kTypeEncoder] + numberDevices[kTypeEncoderSingleDetent]))
        sendFailureMessage("Encoders");
#ifdef MF_LCD_SUPPORT
    if (!LCDDisplay::setupArray(numberDevices[kTypeLcdDisplayI2C]))
        sendFailureMessage("LCD");
#endif
#ifdef MF_ANALOG_SUPPORT
    if (!Analog::setupArray(numberDevices[kTypeAnalogInput] + numberDevices[kTypeAnalogInputDeprecated]))
        sendFailureMessage("AnalogIn");
#endif
#ifdef MF_OUTPUT_SHIFTER_SUPPORT
    if (!OutputShifter::setupArray(numberDevices[kTypeOutputShifter]))
        sendFailureMessage("OutputShifter");
#endif
#ifdef MF_INPUT_SHIFTER_SUPPORT
    if (!InputShifter::setupArray(numberDevices[kTypeInputShifter]))
        sendFailureMessage("InputShifter");
#endif
#ifdef MF_DIGIN_MUX_SUPPORT
    if (!DigInMux::setupArray(numberDevices[kTypeDigInMux]))
        sendFailureMessage("DigInMux");
#endif
#ifdef MF_CUSTOMDEVICE_SUPPORT
    if (!CustomDevice::setupArray(numberDevices[kTypeCustomDevice]))
        sendFailureMessage("CustomDevice");
#endif
    return;
}

void readConfig()
{
    uint8_t numberDevices[kTypeMax] = {0};

    // Early return if no valid configuration is found
    if (!configStoredInFlash() && !configStoredInEEPROM()) {
        InitArrays(numberDevices);
        return;
    }

    // Determine which configuration to use and proceed
    GetArraySizes(numberDevices, configStoredInFlash());
    InitArrays(numberDevices);
    readConfigFromMemory(configStoredInFlash());
}

void readConfigFromMemory(bool configFromFlash)
{
    uint16_t addrMem      = 0;    // define first memory location where config is saved in EEPROM or Flash
    char     params[8]    = "";   // buffer for reading parameters from EEPROM or Flash and sending to ::Add() function of device
    uint8_t  command      = 0;    // read the first value from EEPROM or Flash, it's a device definition
    bool     copy_success = true; // will be set to false if copying input names to nameBuffer exceeds array dimensions
                                  // not required anymore when pins instead of names are transferred to the UI

    if (!configFromFlash) {
        addrMem = MEM_OFFSET_CONFIG;
    }

    // read the first value from EEPROM, it's a device definition
    command = readUint(&addrMem, configFromFlash);

    // go through the EEPROM or Flash until it is NULL terminated
    do {
        switch (command) {
        case kTypeButton:
            params[0] = readUint(&addrMem, configFromFlash);                              // Pin number
            Button::Add(params[0], &nameBuffer[pNameBuffer]);                             // MUST be before readName because readName returns the pointer for the NEXT Name
            copy_success = readName(&addrMem, nameBuffer, &pNameBuffer, configFromFlash); // copy the NULL terminated name to nameBuffer and set to next free memory location
            break;

        case kTypeOutput:
            params[0] = readUint(&addrMem, configFromFlash); // Pin number
            Output::Add(params[0]);
            copy_success = readEndCommand(&addrMem, ':', configFromFlash); // check EEPROM until end of name
            break;

#ifdef MF_SEGMENT_SUPPORT
        // this is for backwards compatibility
        case kTypeLedSegmentDeprecated:
        // this is the new type
        case kTypeLedSegmentMulti:
            params[0] = LedSegment::TYPE_MAX72XX;
            if (command == kTypeLedSegmentMulti)
                params[0] = readUint(&addrMem, configFromFlash); // Type of LedSegment

            params[1] = readUint(&addrMem, configFromFlash); // Pin Data number
            params[2] = readUint(&addrMem, configFromFlash); // Pin CS number
            params[3] = readUint(&addrMem, configFromFlash); // Pin CLK number
            params[4] = readUint(&addrMem, configFromFlash); // brightness
            params[5] = readUint(&addrMem, configFromFlash); // number of modules
            LedSegment::Add(params[0], params[1], params[2], params[3], params[5], params[4]);
            copy_success = readEndCommand(&addrMem, ':', configFromFlash); // check EEPROM until end of name
            break;
#endif

#ifdef MF_STEPPER_SUPPORT
        case kTypeStepperDeprecated1:
        case kTypeStepperDeprecated2:
        case kTypeStepper:
            // Values for all stepper types
            params[0] = readUint(&addrMem, configFromFlash); // Pin1 number
            params[1] = readUint(&addrMem, configFromFlash); // Pin2 number
            params[2] = readUint(&addrMem, configFromFlash); // Pin3 number
            params[3] = readUint(&addrMem, configFromFlash); // Pin4 number

            // Default values for older types
            params[4] = (uint8_t)0; // Button number
            params[5] = (uint8_t)0; // Stepper Mode
            params[6] = (uint8_t)0; // backlash
            params[7] = false;      // deactivate output

            if (command == kTypeStepperDeprecated2 || command == kTypeStepper) {
                params[4] = readUint(&addrMem, configFromFlash); // Button number
            }

            if (command == kTypeStepper) {
                params[5] = readUint(&addrMem, configFromFlash); // Stepper Mode
                params[6] = readUint(&addrMem, configFromFlash); // backlash
                params[7] = readUint(&addrMem, configFromFlash); // deactivate output
            }
            // there is an additional 9th parameter stored in the config (profileID) which is not needed in the firmware
            // and therefor not read in, it is just skipped like the name with reading until end of command
            Stepper::Add(params[0], params[1], params[2], params[3], params[4], params[5], params[6], params[7]);
            copy_success = readEndCommand(&addrMem, ':', configFromFlash); // check EEPROM until end of name
            break;
#endif

#ifdef MF_SERVO_SUPPORT
        case kTypeServo:
            params[0] = readUint(&addrMem, configFromFlash); // Pin number
            Servos::Add(params[0]);
            copy_success = readEndCommand(&addrMem, ':', configFromFlash); // check EEPROM until end of name
            break;
#endif

        case kTypeEncoderSingleDetent:
        case kTypeEncoder:
            params[0] = readUint(&addrMem, configFromFlash); // Pin1 number
            params[1] = readUint(&addrMem, configFromFlash); // Pin2 number
            params[2] = 0;                                   // type

            if (command == kTypeEncoder)
                params[2] = readUint(&addrMem, configFromFlash); // type

            Encoder::Add(params[0], params[1], params[2], &nameBuffer[pNameBuffer]);      // MUST be before readName because readName returns the pointer for the NEXT Name
            copy_success = readName(&addrMem, nameBuffer, &pNameBuffer, configFromFlash); // copy the NULL terminated name to nameBuffer and set to next free memory location
            break;

#ifdef MF_LCD_SUPPORT
        case kTypeLcdDisplayI2C:
            params[0] = readUint(&addrMem, configFromFlash); // address
            params[1] = readUint(&addrMem, configFromFlash); // columns
            params[2] = readUint(&addrMem, configFromFlash); // lines
            LCDDisplay::Add(params[0], params[1], params[2]);
            copy_success = readEndCommand(&addrMem, ':', configFromFlash); // check EEPROM until end of name
            break;
#endif

#ifdef MF_ANALOG_SUPPORT
        case kTypeAnalogInputDeprecated:
        case kTypeAnalogInput:
            params[0] = readUint(&addrMem, configFromFlash);                              // pin number
            params[1] = readUint(&addrMem, configFromFlash);                              // sensitivity
            if (command == kTypeAnalogInputDeprecated)
                Analog::Add(params[0], &nameBuffer[pNameBuffer], params[1], true);        // MUST be before readName because readName returns the pointer for the NEXT Name
            else
                Analog::Add(params[0], &nameBuffer[pNameBuffer], params[1], false);       // MUST be before readName because readName returns the pointer for the NEXT Name
            copy_success = readName(&addrMem, nameBuffer, &pNameBuffer, configFromFlash); // copy the NULL terminated name to to nameBuffer and set to next free memory location
                                                                                          //    copy_success = readEndCommand(&addrMem, ':');       // once the nameBuffer is not required anymore uncomment this line and delete the line before
            break;
#endif

#ifdef MF_OUTPUT_SHIFTER_SUPPORT
        case kTypeOutputShifter:
            params[0] = readUint(&addrMem, configFromFlash); // latch Pin
            params[1] = readUint(&addrMem, configFromFlash); // clock Pin
            params[2] = readUint(&addrMem, configFromFlash); // data Pin
            params[3] = readUint(&addrMem, configFromFlash); // number of daisy chained modules
            OutputShifter::Add(params[0], params[1], params[2], params[3]);
            copy_success = readEndCommand(&addrMem, ':', configFromFlash); // check EEPROM until end of name
            break;
#endif

#ifdef MF_INPUT_SHIFTER_SUPPORT
        case kTypeInputShifter:
            params[0] = readUint(&addrMem, configFromFlash); // latch Pin
            params[1] = readUint(&addrMem, configFromFlash); // clock Pin
            params[2] = readUint(&addrMem, configFromFlash); // data Pin
            params[3] = readUint(&addrMem, configFromFlash); // number of daisy chained modules
            InputShifter::Add(params[0], params[1], params[2], params[3], &nameBuffer[pNameBuffer]);
            copy_success = readName(&addrMem, nameBuffer, &pNameBuffer, configFromFlash); // copy the NULL terminated name to to nameBuffer and set to next free memory location
                                                                                          //    copy_success = readEndCommand(&addrMem, ':');       // once the nameBuffer is not required anymore uncomment this line and delete the line before
            break;
#endif

#ifdef MF_DIGIN_MUX_SUPPORT
        case kTypeDigInMux:
            params[0] = readUint(&addrMem, configFromFlash); // data pin
            // Mux driver section
            // Repeated commands do not define more objects, but change the only existing one
            // therefore beware that all DigInMux configuration commands are consistent!
            params[1] = readUint(&addrMem, configFromFlash); // Sel0 pin
            params[2] = readUint(&addrMem, configFromFlash); // Sel1 pin
            params[3] = readUint(&addrMem, configFromFlash); // Sel2 pin
            params[4] = readUint(&addrMem, configFromFlash); // Sel3 pin
            MUX.attach(params[1], params[2], params[3], params[4]);
            params[5] = readUint(&addrMem, configFromFlash); // 8-bit registers (1-2)
            DigInMux::Add(params[0], params[5], &nameBuffer[pNameBuffer]);
            copy_success = readName(&addrMem, nameBuffer, &pNameBuffer, configFromFlash);
            break;
#endif

#ifdef MF_CUSTOMDEVICE_SUPPORT
        case kTypeCustomDevice: {
            uint16_t adrType = addrMem; // first location of custom Type in EEPROM
            copy_success     = readEndCommand(&addrMem, '.', configFromFlash);
            if (!copy_success)
                break;

            uint16_t adrPin = addrMem; // first location of custom pins in EEPROM
            copy_success    = readEndCommand(&addrMem, '.', configFromFlash);
            if (!copy_success)
                break;

            uint16_t adrConfig = addrMem; // first location of custom config in EEPROM
            copy_success       = readEndCommand(&addrMem, '.', configFromFlash);
            if (copy_success) {
                CustomDevice::Add(adrPin, adrType, adrConfig, configFromFlash);
                copy_success = readEndCommand(&addrMem, ':', configFromFlash); // check EEPROM until end of command
            }
            break;
        }
#endif

        default:
            copy_success = readEndCommand(&addrMem, ':', configFromFlash); // check EEPROM until end of name
        }
        command = readUint(&addrMem, configFromFlash);
    } while (command && copy_success);
    if (!copy_success) {                            // too much/long names for input devices
        nameBuffer[MEMLEN_NAMES_BUFFER - 1] = 0x00; // terminate the last copied (part of) string with 0x00
        cmdMessenger.sendCmd(kStatus, F("Failure on reading config"));
    }
}

void OnGetConfig()
{
    cmdMessenger.sendCmdStart(kInfo);
    if (configStoredInEEPROM()) {
        cmdMessenger.sendCmdArg((char)MFeeprom.read_byte(MEM_OFFSET_CONFIG));
        for (uint16_t i = 1; i < configLengthEEPROM; i++) {
            cmdMessenger.sendArg((char)MFeeprom.read_byte(MEM_OFFSET_CONFIG + i));
        }
    } else if (configStoredInFlash()) {
        cmdMessenger.sendCmdArg((char)pgm_read_byte_near(CustomDeviceConfig));
        for (uint16_t i = 1; i < (configLengthFlash - 1); i++) {
            cmdMessenger.sendArg((char)pgm_read_byte_near(CustomDeviceConfig + i));
        }
    }
    cmdMessenger.sendCmdEnd();
    boardReady = true;
}

void OnGetInfo()
{
    // read the serial number and generate if 1st start up, was before in ResetBoard()
    // moved to this position as the time to generate a serial number in ResetBoard() is always the same
    // OnGetInfo() is called from the connector and the time is very likely always different
    // Therefore millis() can be used for randomSeed
    generateSerial(false);
    cmdMessenger.sendCmdStart(kInfo);
    cmdMessenger.sendCmdArg(F(MOBIFLIGHT_TYPE));
    cmdMessenger.sendCmdArg(name);
    cmdMessenger.sendCmdArg(serial);
    cmdMessenger.sendCmdArg(VERSION);
    cmdMessenger.sendCmdArg(CORE_VERSION);
    cmdMessenger.sendCmdEnd();
}

bool getStatusConfig()
{
    return configActivated;
}

// ************************************************************
// serial number handling
// ************************************************************

// Generate a serial number only for AVR's
#ifdef ARDUINO_ARCH_AVR
void generateRandomSerial()
{
    // To have not always the same starting point for the random generator, millis() are
    // used as starting point. It is very unlikely that the time between flashing the firmware
    // and getting the command to send the info's to the connector is always the same.
    // additional double check if it's really a new board, should reduce Jaimes problem
    randomSeed(millis());
    serial[0]             = 'S';
    serial[1]             = 'N';
    serial[2]             = '-';
    serial[6]             = '-';
    serial[10]            = 0x00;
    uint16_t randomSerial = random(4095);
    for (uint8_t i = 3; i < 6; i++) {
        serial[i] = (randomSerial & 0x000F) + 48; // convert from 4bit to HEX string
        if (serial[i] >= 58) serial[i] += 8;      // if HeX value is A - F add 8 to get the letters
        randomSerial >>= 4;
    }
    randomSerial = random(4095);
    for (uint8_t i = 7; i < 10; i++) {
        serial[i] = (randomSerial & 0x000F) + 48; // convert from 4bit to HEX string
        if (serial[i] >= 58) serial[i] += 7;      // if HeX value is A - F add 7 to get the letters
        randomSerial >>= 4;
    }
    MFeeprom.write_block(MEM_OFFSET_SERIAL, serial, MEM_LEN_SERIAL);
#ifdef DEBUG2CMDMESSENGER
    cmdMessenger.sendCmd(kDebug, F("Serial number generated"));
#endif
}
#endif

#if !defined(ARDUINO_ARCH_AVR)
void readUniqueSerial()
{
    serial[0] = 'S';
    serial[1] = 'N';
    serial[2] = '-';
    for (size_t i = 0; i < UniqueIDsize; i++) {
        serial[3 + i * 2] = (UniqueID[i] >> 4) + 48;
        if (serial[3 + i * 2] >= 58) serial[3 + i * 2] += 7;
        serial[3 + i * 2 + 1] = (UniqueID[i] & 0x0F) + 48;
        if (serial[3 + i * 2 + 1] >= 58) serial[3 + i * 2 + 1] += 7;
    }
}
#endif

void generateSerial(bool force)
{
    if (force) {
        // A serial number is forced to generate
        // generate a serial number acc. the old style only for AVR's
#ifdef ARDUINO_ARCH_AVR
        generateRandomSerial();
#else
        // For other boards always the UniqueID is used.
        readUniqueSerial();
        // If there is always a serial number acc. old style and the user
        // requests a new one, he will get the UniqueID and it's marked in the EEPROM
        if (MFeeprom.read_byte(MEM_OFFSET_SERIAL) == 'S' && MFeeprom.read_byte(MEM_OFFSET_SERIAL + 1) == 'N') {
            MFeeprom.write_byte(MEM_OFFSET_SERIAL, 0x00);
        }
#endif
        return;
    }

    // A serial number according old style is already generated and saved to the eeprom
    // For other boards this is kept for backwards compatibility
    if (MFeeprom.read_byte(MEM_OFFSET_SERIAL) == 'S' && MFeeprom.read_byte(MEM_OFFSET_SERIAL + 1) == 'N') {
        MFeeprom.read_block(MEM_OFFSET_SERIAL, serial, MEM_LEN_SERIAL);
        return;
    }

#ifdef ARDUINO_ARCH_AVR
    // Coming here no serial number is available (so it's the first start up of an AVR board)
    // or a uniqueID is already generated and saved to the eeprom
    // AVR's are forced to roll back to "old style" serial number
    generateRandomSerial();
#else
    // other boards always uses the UniqueID
    readUniqueSerial();
#endif
}

void OnGenNewSerial()
{
    generateSerial(true);
    cmdMessenger.sendCmd(kInfo, serial);
}

// ************************************************************
// Naming handling
// ************************************************************
void storeName()
{
    if (!configStoredInFlash()) {
        MFeeprom.write_byte(MEM_OFFSET_NAME, '#');
        MFeeprom.write_block(MEM_OFFSET_NAME + 1, name, MEM_LEN_NAME - 1);
        // MFeeprom.commit() is not required, name is always set before config
    }
}

void restoreName()
{
    if (MFeeprom.read_byte(MEM_OFFSET_NAME) != '#')
        return;

    MFeeprom.read_block(MEM_OFFSET_NAME + 1, name, MEM_LEN_NAME - 1);
}

void OnSetName()
{
    char *cfg = cmdMessenger.readStringArg();
    if (!configStoredInFlash()) {
        memcpy(name, cfg, MEM_LEN_NAME);
        storeName();
    }
    cmdMessenger.sendCmd(kStatus, name);
}

// config.cpp
