// ============================================================================ //
// Copyright (C) 2010-2024 Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
// ============================================================================ //

#ifdef _MSC_VER

#ifndef CORE_WINAPI_H
#define CORE_WINAPI_H

// -----------------------------------------------------------------------------

// These are headers which are included by Windows.h. We need to beat windows
// to it so they don't get put in the WinApi namespace...
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

namespace WinApi {

#define NOMINMAX
#ifndef NOGDI
#define NOGDI	//Fix problems with the DEFAULT_PITCH register
#endif
#ifndef STRICT
#define STRICT
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <windowsx.h>

//
// These are redefinitions for ugly things that windows does and we can't use
// without being in "namespace WinApi"
//
static const DWORD WaitTimeout = WAIT_TIMEOUT;
static const DWORD WaitObject0 = WAIT_OBJECT_0;
static const DWORD WaitFailed = WAIT_FAILED;

static HKEY const HKeyClassesRoot = HKEY_CLASSES_ROOT;
static HKEY const HKeyCurrentUser = HKEY_CURRENT_USER;
static HKEY const HKeyLocalMachine = HKEY_LOCAL_MACHINE;
static HKEY const HKeyUsers = HKEY_USERS;

static inline WORD MakeLangID(USHORT arg1, USHORT arg2) { return MAKELANGID(arg1, arg2); }

static HANDLE const InvalidHandleValue = INVALID_HANDLE_VALUE;

static const int ThreadPriorityTimeCritical = THREAD_PRIORITY_TIME_CRITICAL;
static const int ThreadPriorityNormal = THREAD_PRIORITY_NORMAL;

static const int MbOk = MB_OK;
static const int MbIconError = MB_ICONERROR;

}; // namespace WinApi

// -----------------------------------------------------------------------------

#endif

#endif
