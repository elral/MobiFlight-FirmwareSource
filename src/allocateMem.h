//
// allocatemem.h
//
// (C) MobiFlight Project 2022
//
// 
// Single Object, avoids calling 'placement new' with nullptr
//     void* mem = MF_ALLOC_TYPE(AccelStepper, 1);
//     if (mem) {
//         _stepper = new (mem) AccelStepper(AccelStepper::DRIVER, pin1, pin2);
//     } else {
//         // handle error
//         return;
//     }
// 
// Object array
//     _encoders = static_cast<MFEncoder*>(MF_ALLOC_TYPE(MFEncoder, count));
// 
// uint8_t-Array
//     _buffer = static_cast<uint8_t*>(MF_ALLOC_BYTES(count));
// 
// uint16_t-Array
//     _values = static_cast<uint16_t*>(MF_ALLOC_TYPE(uint16_t, count));
// 
// uint32_t-Array
//     _values = static_cast<uint32_t*>(MF_ALLOC_TYPE(uint32_t, count));
// 

#pragma once

#include <Arduino.h>
#include <new>

#define MF_ALLOC_BYTES(count)   allocateMemory((size_t)(count), 1)
#define MF_ALLOC_TYPE(T, count) allocateMemory((size_t)(sizeof(T) * (count)), alignof(T))

void  *allocateMemory(size_t size, size_t alignment = alignof(max_align_t));
void   ClearMemory();
size_t GetAvailableMemory();

// allocatemem.h
