#include "mobiflight.h"
#include "CustomDevice.h"
#include "MFCustomDevice.h"
#if defined(USE_2ND_CORE)
#include <FreeRTOS.h>
#endif

/* **********************************************************************************
    Normally nothing has to be changed in this file
    It handles one or multiple custom devices
********************************************************************************** */

#define MESSAGEID_POWERSAVINGMODE -2

namespace CustomDevice
{
    MFCustomDevice *customDevice;
    uint8_t         customDeviceRegistered = 0;
    uint8_t         maxCustomDevices       = 0;
#if defined(USE_2ND_CORE)
    char payload[SERIAL_RX_BUFFER_SIZE];
#endif

    bool setupArray(uint16_t count)
    {
        if (!FitInMemory(sizeof(MFCustomDevice) * count))
            return false;
        customDevice     = new (allocateMemory(sizeof(MFCustomDevice) * count)) MFCustomDevice();
        maxCustomDevices = count;
        return true;
    }

    void Add(uint16_t adrPin, uint16_t adrType, uint16_t adrConfig, bool configFromFlash)
    {
        if (customDeviceRegistered == maxCustomDevices)
            return;
        customDevice[customDeviceRegistered] = MFCustomDevice();
        customDevice[customDeviceRegistered].attach(adrPin, adrType, adrConfig, configFromFlash);
        customDeviceRegistered++;
#ifdef DEBUG2CMDMESSENGER
        cmdMessenger.sendCmd(kStatus, F("Added CustomDevice"));
#endif
    }

    /* **********************************************************************************
        All custom devives gets unregistered if a new config gets uploaded.
        The Clear() function from every registerd custom device will be called.
        Keep it as it is, mostly nothing must be changed
    ********************************************************************************** */
    void Clear()
    {
        for (int i = 0; i != customDeviceRegistered; i++) {
            customDevice[i].detach();
        }
        customDeviceRegistered = 0;
#ifdef DEBUG2CMDMESSENGER
        cmdMessenger.sendCmd(kStatus, F("Cleared CustomDevice"));
#endif
    }

    /* **********************************************************************************
        Within in loop() the update() function is called regularly
        Within the loop() you can define a time delay where this function gets called
        or as fast as possible. See comments in loop()
        The update() function from every registerd custom device will be called.
        It is only needed if you have to update your custom device without getting
        new values from the connector. Otherwise just make a return;
    ********************************************************************************** */
    void update()
    {
#if !defined(USE_2ND_CORE)
        for (int i = 0; i != customDeviceRegistered; i++) {
            customDevice[i].update();
        }
#endif
    }

    /* **********************************************************************************
        If an output for the custom device is defined in the connector,
        this function gets called when a new value is available.
        In this case the connector sends a messageID followed by a string which is not longer
        than 90 Byte (!!check the length of the string!!) limited by the commandMessenger.
        The messageID is used to mark the value which has changed. This reduced the serial
        communication as not all values has to be send in one (big) string (like for the LCD)
        The OnSet() function from every registerd custom device will be called.
    ********************************************************************************** */
    void OnSet()
    {
        int16_t device = cmdMessenger.readInt16Arg(); // get the device number
        if (device >= customDeviceRegistered)         // and do nothing if this device is not registered
            return;
        int16_t messageID = cmdMessenger.readInt16Arg();  // get the messageID number
        char   *output    = cmdMessenger.readStringArg(); // get the pointer to the new raw string
        cmdMessenger.unescape(output);                    // and unescape the string if escape characters are used
#if defined(USE_2ND_CORE)
        // copy the message, could get be overwritten from the next message while processing on 2nd core
        strncpy(payload, output, SERIAL_RX_BUFFER_SIZE);
        // wait for 2nd core
        rp2040.fifo.pop();
        // Hmhm, how to get the function pointer to a function from class??
        // rp2040.fifo.push((uintptr_t) &customDevice[device].set);
        rp2040.fifo.push(device);
        rp2040.fifo.push(messageID);
        rp2040.fifo.push((uint32_t)&payload);
#else
        customDevice[device].set(messageID, output); // send the string to your custom device
#endif
    }

    /* **********************************************************************************
        This function is called if the status of the PowerSavingMode changes.
        'state' is true if PowerSaving is enabled
        'state' is false if PowerSaving is disabled
        MessageID '-2' for the custom device  for PowerSavingMode
    ********************************************************************************** */
    void PowerSave(bool state)
    {
        for (uint8_t i = 0; i < customDeviceRegistered; ++i) {
            if (state)
                customDevice[i].set(MESSAGEID_POWERSAVINGMODE, (char*)"1");
            else
                customDevice[i].set(MESSAGEID_POWERSAVINGMODE, (char*)"0");
        }
    }
} // end of namespace

#if defined(USE_2ND_CORE)
/* **********************************************************************************
    This will run the set() function from the custom device on the 2nd core
    Be aware NOT to use the function calls from the Pico SDK!
    Only use the functions from the used framework from EarlePhilHower
    If you mix them up it will give undefined behaviour and strange effects
    see https://arduino-pico.readthedocs.io/en/latest/multicore.html
********************************************************************************** */
void setup1()
{
    // send "ready" message to 1st core
    rp2040.fifo.push(true);
}

void loop1()
{
    uint8_t device;
    int16_t messageID;
    char   *payload;
#ifdef MF_CUSTOMDEVICE_POLL_MS
    uint32_t lastMillis = 0;
#endif

    while (1) {
#ifndef MF_CUSTOMDEVICE_POLL_MS
        // For now I don't know the reason why this is required.
        // It might be that getting the stop command from the 1st core
        // needs some idle time. If it is not used the 1st core stops when
        // writing to the EEPROM which stops the 2nd core
        delayMicroseconds(1);
#endif  
#ifdef MF_CUSTOMDEVICE_POLL_MS
        if (millis() - lastMillis >= MF_CUSTOMDEVICE_POLL_MS) {
#endif
            for (int i = 0; i != CustomDevice::customDeviceRegistered; i++) {
#if defined(MF_CUSTOMDEVICE_HAS_UPDATE)
                CustomDevice::customDevice[i].update();
#endif
                if (rp2040.fifo.available() == 3) {
                    // Hmhm, how to get the function pointer to a function from class??
                    // int32_t (*func)(int16_t, char*) = (int32_t(*)(int16_t, char*)) rp2040.fifo.pop();
                    device    = (uint8_t)rp2040.fifo.pop();
                    messageID = (int16_t)rp2040.fifo.pop();
                    payload   = (char *)rp2040.fifo.pop();
                    // (*func)(messageID, payload);
                    CustomDevice::customDevice[device].set(messageID, payload);
                    // send ready for next message to 1st core
                    rp2040.fifo.push(true);
                }

            }
#ifdef MF_CUSTOMDEVICE_POLL_MS
            lastMillis = millis();
        }
#endif
    }
}
#endif
