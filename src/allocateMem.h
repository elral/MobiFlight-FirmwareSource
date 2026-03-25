//
// allocatemem.h
//
// (C) MobiFlight Project 2022
//

#pragma once

#include <Arduino.h>
#include <new>

#ifdef ARDUINO_ARCH_AVR
void    *allocateMemory(uint16_t size);
void     ClearMemory();
uint16_t GetAvailableMemory();
bool     FitInMemory(uint16_t size);
#else
void  *allocateMemory(size_t size, size_t alignment = alignof(max_align_t));
void   ClearMemory();
size_t GetAvailableMemory();
bool   FitInMemory(size_t size, size_t alignment = alignof(max_align_t));

template <typename T, typename... Args>
// Class with Default-Konstruktor
T *allocateObject(Args &&...args)
{
    void *mem = allocateMemory(sizeof(T), alignof(T));
    if (!mem) return nullptr;
    return new (mem) T(static_cast<Args &&>(args)...);
}

// Array of Classes with Default-Konstruktor
template <typename T>
T *allocateArray(size_t count)
{
    if (count == 0) return nullptr;

    void *mem = allocateMemory(sizeof(T) * count, alignof(T));
    if (!mem) return nullptr;

    T *arr = static_cast<T *>(mem);

    for (size_t i = 0; i < count; ++i) {
        new (&arr[i]) T();
    }

    return arr;
}

// Array of Classes with all Elements of same Arguments
template <typename T, typename... Args>
T *allocateArray(size_t count, Args... args)
{
    if (count == 0) return nullptr;

    void *mem = allocateMemory(sizeof(T) * count, alignof(T));
    if (!mem) return nullptr;

    T *arr = static_cast<T *>(mem);

    for (size_t i = 0; i < count; ++i) {
        new (&arr[i]) T(args...);
    }

    return arr;
}
#endif
// allocatemem.h
