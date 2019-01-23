#pragma once

#include <stdint.h>

// An eight-bit signed type
typedef signed char sByte;

// An eight-bit unsigned type
typedef unsigned char uByte;

// A sixteen-bit signed type
typedef signed short s2Byte;

// A sixteen-bit unsigned type
typedef unsigned short u2Byte;

// A thirty-two bit signed type
typedef signed int s4Byte;

// A thirty-two bit unsigned type
typedef unsigned int u4Byte;

// A sixty-four bit signed type
typedef signed long long s8Byte;

// A sixty-four bit unsigned type
typedef unsigned long long u8Byte;

// A pointer alias (length depends on compilation target)
typedef void* address;