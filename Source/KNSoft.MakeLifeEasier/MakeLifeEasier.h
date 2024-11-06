﻿/*
 * KNSoft.MakeLifeEasier (https://github.com/KNSoft/KNSoft.MakeLifeEasier)
 * Copyright (c) KNSoft.org (https://github.com/KNSoft). All rights reserved.
 * Licensed under the MIT license.
 *
 * A fast, stable and convenient C/C++ library
 * to "Make Life Easier" when suffering from Windows NT development.
 */

#pragma once

#include <KNSoft/NDK/NDK.h>

#ifndef MLE_API
#define MLE_API DECLSPEC_IMPORT
#endif

typedef _Return_type_success_(return == 0) ULONG W32ERROR;

/*
 * Core Header
 * - Provides core functions
 * - Depends on NDK only
 * - Header only
 */
#include "Memory/Core.h"
#include "String/Core.h"

/*
 * L1 Header
 * - Provides basic and low-level utilities
 * - Depends on NDK and L0 header only
 */
#include "Error/Code.h"
#include "Math/Math.h"
#include "Math/Rand.h"
#include "Memory/Memory.h"
#include "NT/Object.h"
#include "Process/Sync.h"
#include "Time/Time.h"
#include "String/Convert.h"
#include "String/Hash.h"
#include "System/Library.h"
#include "System/Registry.h"
#include "UI/GDI.h"

/* L2 Header: No dependencies yet */
#include "Crypt/Cert.h"
#include "IO/Hardware.h"
#include "NT/Security.h"
#include "PE/Resolve.h"
#include "PE/Resource.h"
#include "Process/Loader.h"
#include "Process/Token.h"
#include "Shell/Shell.h"
#include "System/Info.h"
#include "UI/Control.h"
#include "UI/Dialog/Dialog.h"
#include "UI/DPI.h"
#include "UI/Message.h"
#include "UI/Paint.h"
#include "UI/Window.h"

/* L3 Header: Depends on above headers */
#include "Error/Message.h"
#include "IO/File.h"
