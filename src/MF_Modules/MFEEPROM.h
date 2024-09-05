//
// MFEEPROM.h
//
// (C) MobiFlight Project 2022
//

#pragma once

#include <EEPROM.h>

class MFEEPROM
{
private:
    uint16_t _eepromLength = 0;

public:
    MFEEPROM();
    void     init(void);
    uint16_t get_length(void);
    uint8_t read_byte(uint16_t adr);
    bool write_byte(uint16_t adr, const uint8_t data);
    void commit() {
#if (!defined(ARDUINO_ARCH_AVR) && !defined(ARDUINO_ARCH_STM32))
        EEPROM.commit();
#endif
#if defined(ARDUINO_ARCH_STM32)
        eeprom_buffer_flush();
#endif
    }

    template <typename T>
    bool read_block(uint16_t adr, T &t)
    {
        if (adr + sizeof(T) > _eepromLength) return false;
#if defined(ARDUINO_ARCH_STM32)
        uint8_t *ptr = (uint8_t *) &t;
        for (int count = sizeof(T) ; count ; --count) {
            *ptr++ = eeprom_buffered_read_byte(adr + count);
        }
#else
        EEPROM.get(adr, t);
#endif
        return true;
    }

    template <typename T>
    bool read_block(uint16_t adr, T &t, uint16_t len)
    {
        if (adr + len > _eepromLength) return false;
        uint8_t *ptr = (uint8_t*) &t;
        for (uint16_t i = 0; i < len; i++) {
#if defined(ARDUINO_ARCH_STM32)
            *ptr++ = eeprom_buffered_read_byte(adr + i);
#else
            *ptr++ = EEPROM.read(adr + i);
#endif
        }
        return true;
    }

    template <typename T>
    const bool write_block(uint16_t adr, const T &t)
    {
        if (adr + sizeof(T) > _eepromLength) return false;
#if defined(ARDUINO_ARCH_STM32)
        const uint8_t *ptr = (const uint8_t *) &t;
        for (int count = sizeof(T) ; count ; --count) {
            eeprom_buffered_write_byte(adr + count, *ptr++);
        }
#else
        EEPROM.put(adr, t);
#endif
        return true;
    }

    template <typename T>
    const bool write_block(uint16_t adr, const T &t, uint16_t len)
    {
        if (adr + len > _eepromLength) return false;
        for (uint16_t i = 0; i < len; i++) {
#if defined(ARDUINO_ARCH_STM32)
            eeprom_buffered_write_byte(adr + i, t[i]);
#else
            EEPROM.put(adr + i, t[i]);
#endif
        }
        return true;
    }
};

// MFEEPROM.h