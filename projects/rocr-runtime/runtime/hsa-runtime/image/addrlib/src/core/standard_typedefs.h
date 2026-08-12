// Copyright (c) 2001-2025 Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT

//****************************************************************************************
//
// TITLE: Standard "typedef"s
//
// DESCRIPTION: This file contains very generic "typedef"s that any library can include.
//
//              PLEASE, UPDATE THIS FILE JUDICIOUSLY.
//
//****************************************************************************************

#ifndef _STANDARD_TYPEDEFS_DEFINED_
#define _STANDARD_TYPEDEFS_DEFINED_

//----------------------------------------------------------------------------------------
// Define sized-based typedefs up to 32-bits.
//----------------------------------------------------------------------------------------
typedef signed char             int8;
typedef unsigned char           uint8;

typedef signed short            int16;
typedef unsigned short          uint16;

typedef signed int              int32;
typedef unsigned int            uint32;

//----------------------------------------------------------------------------------------
// Define 64-bit typedefs, depending on the compiler and operating system.
//----------------------------------------------------------------------------------------
#ifdef __GNUC__
typedef long long               int64;
typedef unsigned long long      uint64;

#else                                        // not __GNUC__
#ifdef _WIN32
typedef __int64                 int64;
typedef unsigned __int64        uint64;

#else                                        // not _WIN32
#error Unsupported compiler and/or operating system
#endif                                       // end ifdef _WIN32

#endif                                       // end ifdef __GNUC__

//----------------------------------------------------------------------------------------
// Define other generic typedefs.
//----------------------------------------------------------------------------------------
typedef unsigned int            uint;
typedef unsigned long           ulong;

//****************************************************************************************
// End of _STANDARD_TYPEDEFS_DEFINED_
//****************************************************************************************
#endif
