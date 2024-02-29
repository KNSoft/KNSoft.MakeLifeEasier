#pragma once

#include "../CRT.inl"

EXTERN_C_START

void __cdecl _initterm(_PVFV* const first, _PVFV* const last);
int __cdecl _initterm_e(_PIFV* const first, _PIFV* const last);

NTSTATUS CRT_Startup_Init();
VOID CRT_Startup_Uninit();

VOID Startup_InitCmdlineW(); // Initialize _wcmdln
VOID Startup_InitCmdlineA(); // Initialize _acmdln

VOID Startup_InitCmdlineArgW(); // Initialize __argc and __wargv
VOID Startup_InitCmdlineArgA(); // Initialize __argc and __argv

VOID Startup_FreeCmdlineA();
VOID Startup_FreeCmdlineArgW();
VOID Startup_FreeCmdlineArgA();

EXTERN_C_END
