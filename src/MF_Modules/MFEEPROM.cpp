//
// MFEEPROM.cpp
//
// (C) MobiFlight Project 2022
//

#include <Arduino.h>
#include "MFEEPROM.h"
#include "MFBoards.h"

#if defined(DATA_EEPROM_BASE)
#error "STM32 devices have an integrated EEPROM. No buffered API available."
#endif

MFEEPROM::MFEEPROM() {}

void MFEEPROM::init(void)
{
#if (!defined(ARDUINO_ARCH_AVR) && !defined(ARDUINO_ARCH_STM32))
    EEPROM.begin(4096);
#endif
#if defined(ARDUINO_ARCH_STM32)
    eeprom_buffer_fill();
#endif
    _eepromLength = EEPROM.length();
}

uint16_t MFEEPROM::get_length(void)
{
    return _eepromLength;
}

uint8_t MFEEPROM::read_byte(uint16_t adr)
{
    if (adr >= _eepromLength) return 0;
#if defined(ARDUINO_ARCH_STM32)
    return eeprom_buffered_read_byte(adr);
#else
    return EEPROM.read(adr);
#endif
}

bool MFEEPROM::write_byte(uint16_t adr, const uint8_t data)
{
    if (adr >= _eepromLength) return false;
#if defined(ARDUINO_ARCH_STM32)
    eeprom_buffered_write_byte(adr, data);
#else
    EEPROM.write(adr, data);
#endif
    return true;
}

// MFEEPROM.cpp