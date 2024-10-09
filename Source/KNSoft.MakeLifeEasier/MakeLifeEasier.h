/*
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

#include "Error/Message.h"
#include "IO/Device.h"
#include "IO/File.h"
#include "Math/Math.h"
#include "Math/Rand.h"
#include "Memory/Memory.h"
#include "PE/PE.h"
#include "String/Basic.h"
#include "String/Convert.h"
#include "String/Hash.h"
#include "Shell/Shell.h"
#include "System/Config.h"
#include "System/Library.h"
#include "Time/Time.h"
#include "UI/Dialog/Dialog.h"
#include "UI/Dialog.h"
#include "UI/DPI.h"
#include "UI/GDI.h"
#include "UI/Message.h"
#include "UI/Paint.h"
#include "UI/Window.h"
