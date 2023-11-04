#ifndef MFBoardMicro_h
#define MFBoardMicro_h

#ifndef MF_SEGMENT_SUPPORT
#define MF_SEGMENT_SUPPORT 1
#endif
#ifndef MF_LCD_SUPPORT
#define MF_LCD_SUPPORT 1
#endif
#ifndef MF_STEPPER_SUPPORT
#define MF_STEPPER_SUPPORT 1
#endif
#ifndef MF_SERVO_SUPPORT
#define MF_SERVO_SUPPORT 1
#endif
#ifndef MF_ANALOG_SUPPORT
#define MF_ANALOG_SUPPORT 1
#endif
#ifndef MF_OUTPUT_SHIFTER_SUPPORT
#define MF_OUTPUT_SHIFTER_SUPPORT 1
#endif
#ifndef MF_INPUT_SHIFTER_SUPPORT
#define MF_INPUT_SHIFTER_SUPPORT 1
#endif
#ifndef MF_MUX_SUPPORT
#define MF_MUX_SUPPORT 1
#endif
#ifndef MF_DIGIN_MUX_SUPPORT
#define MF_MUX_SUPPORT       1
#define MF_DIGIN_MUX_SUPPORT 1
#endif
#ifndef MF_CUSTOMDEVICE_SUPPORT
#define MF_CUSTOMDEVICE_SUPPORT 1
#endif
#ifndef MF_KEYMATRIX_SUPPORT
#define MF_KEYMATRIX_SUPPORT 1
#endif
#ifndef MF_KEYMATRIX_SUPPORT
#define MF_KEYMATRIX_SUPPORT 1
#endif
#ifndef MAX_KEYMATRIX
#define MAX_KEYMATRIX        1
#define MAX_COLUMN_KEYMATRIX 8
#define MAX_ROW_KEYMATRIX    8
#endif

#ifndef MOBIFLIGHT_TYPE
#define MOBIFLIGHT_TYPE         "MobiFlight RaspiPico"
#endif
#ifndef MOBIFLIGHT_SERIAL
#define MOBIFLIGHT_SERIAL       "0987654321"
#endif
#ifndef MOBIFLIGHT_NAME
#define MOBIFLIGHT_NAME         "MobiFlight RaspiPico"
#endif
#ifndef MEMLEN_CONFIG
#define MEMLEN_CONFIG           1496    // MUST be less than EEPROM_SIZE!! MEM_OFFSET_CONFIG + MEM_LEN_CONFIG <= EEPROM_SIZE, see: eeprom_write_block (MEM_OFFSET_CONFIG, configBuffer, MEM_LEN_CONFIG);
#endif
#ifndef MEMLEN_NAMES_BUFFER
#define MEMLEN_NAMES_BUFFER     1000    // max. size for configBuffer, contains only names from inputs
#endif
#ifndef MF_MAX_DEVICEMEM
#define MF_MAX_DEVICEMEM        1000    // max. memory size for devices
#endif

#define RANDOM_SEED_INPUT   A0      // better to have randomSeed(analogRead(RANDOM_SEED_INPUT)); in generateSerial(), "0" is not valid for all HW platforms

#endif
