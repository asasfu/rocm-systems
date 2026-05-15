// ============================================================================ //
// Copyright (C) 2010-2025 Advanced Micro Devices, Inc. All rights reserved.
// SPDX-License-Identifier: MIT
// ============================================================================ //

#ifndef _MSC_VER

#ifndef CORE_UNIXAPI_H
#define CORE_UNIXAPI_H

// -----------------------------------------------------------------------------

// These are headers which are included by Windows.h. We need to beat windows
// to it so they don't get put in the UnixApi namespace...
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define NOMINMAX
#define STRICT
#include <sys/types.h>
#include <errno.h>


namespace UnixApi {

typedef int HKEY;

#define HKEY_CLASSES_ROOT 0
#define HKEY_CURRENT_USER 1
#define HKEY_LOCAL_MACHINE 2
#define HKEY_USERS 3

static HKEY const HKeyClassesRoot = HKEY_CLASSES_ROOT;
static HKEY const HKeyCurrentUser = HKEY_CURRENT_USER;
static HKEY const HKeyLocalMachine = HKEY_LOCAL_MACHINE;
static HKEY const HKeyUsers = HKEY_USERS;

}; // namespace UnixApi

// -----------------------------------------------------------------------------

#endif

#endif
