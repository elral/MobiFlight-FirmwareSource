//
// MFAnalog.cpp
//
// (C) MobiFlight Project 2022
//

#include "MFAnalog.h"

#ifdef USE_MCP3008
#include "MCP_ADC.h"
MCP3008 _mcp1((uint8_t)MCP3008_DIN, (uint8_t)MCP3008_DOUT, (uint8_t)MCP3008_CLK);
MCP3008 _mcp2((uint8_t)MCP3008_DIN, (uint8_t)MCP3008_DOUT, (uint8_t)MCP3008_CLK);
bool    _mcp_Initialized = false;
#endif

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
#ifdef USE_MCP3008
    _pin -= 100;
    if (!_mcp_Initialized) {
        _mcp1.begin(MCP3008_CS0);
        _mcp2.begin(MCP3008_CS1);
        _mcp_Initialized = true;
    }
#else
#ifdef ARDUINO_AVR_PROMICRO16
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
#endif
    // Fill averaging buffers with initial reading
    for (uint8_t i = 0; i < ADC_MAX_AVERAGE; i++) {
        readBuffer();
    }
    // and set initial value from buffers
    _lastValue   = ADC_Average_Total >> ADC_MAX_AVERAGE_LOG2;
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

void MFAnalog::readBuffer()
{
    if (!_initialized)
        return;                                             // read ADC and calculate floating average, call it every ~10ms
    ADC_Average_Total -= ADC_Buffer[(ADC_Average_Pointer)]; // subtract oldest value to save the newest value
#ifdef USE_MCP3008
    if (_pin < 8)
        ADC_Buffer[ADC_Average_Pointer] = _mcp1.read(_pin); //_mcp1.readMultiple() required??
    else
        ADC_Buffer[ADC_Average_Pointer] = _mcp2.read(_pin - 7);
#else
    ADC_Buffer[ADC_Average_Pointer] = analogRead(_pin); // store read in, must be subtracted in next loop
#endif
    ADC_Average_Total += ADC_Buffer[ADC_Average_Pointer]; // add read in for floating average
    ADC_Average_Pointer++;                                // prepare for next loop
    ADC_Average_Pointer &= (ADC_MAX_AVERAGE - 1);         // limit max. values for floating average
}

void MFAnalog::attachHandler(analogEvent newHandler)
{
    _handler = newHandler;
}

// MFAnalog.cpp