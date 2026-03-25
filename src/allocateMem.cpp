//
// allocatemem.cpp
//
// (C) MobiFlight Project 2022
//

#include "allocateMem.h"
#include "commandmessenger.h"

alignas(max_align_t) static uint8_t deviceBuffer[MF_MAX_DEVICEMEM] = {0};
static size_t nextPointer                                          = 0;

static size_t alignUp(size_t value, size_t alignment)
{
    return (value + alignment - 1u) & ~(alignment - 1u);
}

void *allocateMemory(size_t size, size_t alignment)
{
    if (size == 0) return nullptr;
    if (alignment == 0) alignment = 1;

    size_t actualPointer = alignUp(nextPointer, alignment);
    size_t newPointer    = actualPointer + size;

    if (newPointer > MF_MAX_DEVICEMEM) {
        cmdMessenger.sendCmd(kStatus, F("DeviceBuffer Overflow!"));
        return nullptr;
    }
    nextPointer = newPointer;
#ifndef DEBUG2CMDMESSENGER
    cmdMessenger.sendCmdStart(kDebug);
    cmdMessenger.sendCmdArg(F("Bytes added"));
    cmdMessenger.sendCmdArg(size);
    cmdMessenger.sendCmdEnd();
    cmdMessenger.sendCmdStart(kDebug);
    cmdMessenger.sendCmdArg(F("BufferUsage"));
    cmdMessenger.sendCmdArg(nextPointer);
    cmdMessenger.sendCmdEnd();
#endif
    return &deviceBuffer[actualPointer];
}

void ClearMemory()
{
    nextPointer = 0;
}

size_t GetAvailableMemory()
{
    return MF_MAX_DEVICEMEM - nextPointer;
}

bool FitInMemory(size_t size, size_t alignment)
{
    if (size == 0) return true;
    if (alignment == 0) alignment = 1;

    size_t actualPointer = alignUp(nextPointer, alignment);
    size_t newPointer    = actualPointer + size;

    return (newPointer <= MF_MAX_DEVICEMEM);
}

// allocatemem.cpp
