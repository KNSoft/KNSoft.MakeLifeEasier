#pragma once

#include "../MakeLifeEasier.h"

EXTERN_C_START

#define Dlg_MsgBox(Owner, Text, Title, Type) MessageBoxTimeoutW(Owner, Text, Title, Type, 0, -1)

EXTERN_C_END
