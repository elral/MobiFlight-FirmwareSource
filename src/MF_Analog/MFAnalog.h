//
// MFAnalog.h
//
// (C) MobiFlight Project 2022
//

#pragma once

#include <Arduino.h>

// Following value defines the buffer size for samples; the larger the buffer,
// the smoother the response (and the larger the delay).
// Buffer size is 2^ADC_MAX_AVERAGE_LOG2: 3 -> 8 samples, 4 -> 16 samples etc.
#define ADC_MAX_AVERAGE_LOG2 3
#define ADC_MAX_AVERAGE      (1 << ADC_MAX_AVERAGE_LOG2)
#define CALIBRATION_START_ADRESS (EEPROM_SIZE - (MAX_ANALOG_PIN * 4) - 1) // base adress for storing calibration data, each calibration data needs 4 Byte (2 * uint16_t)
#define CALIBRATION_TIME         5000                                     // time for calibration, within this time pot must be moved from min to max several time

extern "C" {
// callback functions
typedef void (*analogEvent)(int, const char *);
};

/////////////////////////////////////////////////////////////////////
/// \class MFAnalog MFAnalog.h <MFAnalog.h>
class MFAnalog
{
public:
    MFAnalog();
    static void attachHandler(analogEvent handler);
    void        attach(uint8_t pin, const char *name, uint8_t sensitivity, bool deprecated);
    void        update();
    void        retrigger();
    void        readBuffer();
    void        doCalibration();
    void        readCalibration();
    const char *_name;
    uint8_t     _pin;

private:
    static analogEvent _handler;
    int                _lastValue;
    uint8_t            _sensitivity;

    uint16_t         ADC_Buffer[ADC_MAX_AVERAGE] = {0}; // Buffer for all values from each channel
    uint16_t         ADC_Average_Total           = 0;   // sum of sampled values, must be divided by ADC_MAX_AVERAGE to get actual value
    volatile uint8_t ADC_Average_Pointer         = 0;   // points to the actual position in ADC_BUFFER
    uint32_t         _lastReadBuffer;
    bool             _initialized;

    struct {
        uint16_t minValue = 512;
        uint16_t maxValue = 512;
    } CalibrationData;

    void readChannel(uint8_t compare);
    bool valueHasChanged(int16_t newValue);
};

// MFAnalog.h