//
// MFAnalog.cpp
//
// (C) MobiFlight Project 2022
//

#include "MFAnalog.h"
#include "MFEEPROM.h"
#include "commandmessenger.h"

extern MFEEPROM MFeeprom;

analogEvent MFAnalog::_handler = NULL;

MFAnalog::MFAnalog()
{
    _initialized = false;
}

void MFAnalog::attach(uint8_t pin, const char *name, uint8_t sensitivity, bool deprecated)
{
    _sensitivity = sensitivity;
    _pin         = pin;
    _name        = name;
#if defined(ARDUINO_AVR_PROMICRO16)
    // ProMicro has a special pin assignment for analog pins
    // therefore reading from A6 and A7 does not work
    // via "digital" pins. See also pins_arduino.h
    if (_pin == 4)
        _pin = A6;
    else if (_pin == 6)
        _pin = A7;
#endif
    // enabling PullUp makes a nonlinear behaviour if pot is used
    if (deprecated)
        pinMode(_pin, INPUT_PULLUP);
        
    // Fill averaging buffers with initial reading
    for (uint8_t i = 0; i < ADC_MAX_AVERAGE; i++) {
        readBuffer();
    }
    // and set initial value from buffers
    _lastValue = ADC_Average_Total >> ADC_MAX_AVERAGE_LOG2;
    // readCalibration();
    _initialized = true;
}

bool MFAnalog::valueHasChanged(int16_t newValue)
{
    if (!_initialized)
        return false;
    return (abs(newValue - _lastValue) >= _sensitivity);
}

void MFAnalog::readChannel(uint8_t alwaysTrigger)
{
    if (!_initialized)
        return;
    int16_t newValue = ADC_Average_Total >> ADC_MAX_AVERAGE_LOG2;
    if (alwaysTrigger || valueHasChanged(newValue)) {
//        newValue = map(newValue, CalibrationData.minValue, CalibrationData.maxValue, 0, 1023); // just for testing for now
        _lastValue = newValue;
        if (_handler != NULL) {
            (*_handler)(_lastValue, _name);
        }
    }
}

void MFAnalog::update()
{
    readChannel(false);
}

void MFAnalog::retrigger()
{
    readChannel(true);
}

void MFAnalog::doCalibration()
{
    uint32_t startMillis = millis();
    uint16_t actualValue = 0;

    // read in the min and max value, poti must be moved
    while (startMillis + CALIBRATION_TIME > millis()) {
        // fill the average buffer every 10ms as we are in a while loop and it is not done in the main loop()
        if (!(millis() % 10))
            readBuffer();
        actualValue = ADC_Average_Total >> ADC_MAX_AVERAGE_LOG2;
        if (actualValue < CalibrationData.minValue)
            CalibrationData.minValue = actualValue;
        if (actualValue > CalibrationData.maxValue)
            CalibrationData.maxValue = actualValue;
    }
    // store min and max value to EEPROM, consider pin number for EEPROM adress
    uint16_t calibrationStartAdress = MFeeprom.get_length() - (MAX_ANALOG_PIN * 4) - 1;
    MFeeprom.write_block(calibrationStartAdress + (_pin - FIRST_ANALOG_PIN) * sizeof(CalibrationData), CalibrationData);
}

void MFAnalog::readCalibration()
{
    uint16_t calibrationStartAdress = MFeeprom.get_length() - (MAX_ANALOG_PIN * 4) - 1;
    MFeeprom.read_block(calibrationStartAdress + (_pin - FIRST_ANALOG_PIN) * sizeof(CalibrationData), CalibrationData);
    // check if calibration has been done, otherwise use max. range
    if (CalibrationData.minValue > 1023 || CalibrationData.maxValue > 1023 || CalibrationData.maxValue <= CalibrationData.minValue) {
        CalibrationData.minValue = 0;
        CalibrationData.maxValue = 1023;
    }
    cmdMessenger.sendCmdStart(kReadAnalogCalibration);
    cmdMessenger.sendCmdArg(_name);
    cmdMessenger.sendCmdArg(CalibrationData.minValue);
    cmdMessenger.sendCmdArg(CalibrationData.maxValue);
    cmdMessenger.sendCmdEnd();
}

void MFAnalog::readBuffer()
{                          
    if (!_initialized)
        return;                                 // read ADC and calculate floating average, call it every ~10ms
    ADC_Average_Total -= ADC_Buffer[(ADC_Average_Pointer)]; // subtract oldest value to save the newest value
    ADC_Buffer[ADC_Average_Pointer] = analogRead(_pin);     // store read in, must be subtracted in next loop
    ADC_Average_Total += ADC_Buffer[ADC_Average_Pointer];   // add read in for floating average
    ADC_Average_Pointer++;                                  // prepare for next loop
    ADC_Average_Pointer &= (ADC_MAX_AVERAGE - 1);           // limit max. values for floating average
}

void MFAnalog::attachHandler(analogEvent newHandler)
{
    _handler = newHandler;
}

// MFAnalog.cpp