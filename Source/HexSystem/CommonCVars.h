#pragma once

#include "HAL/IConsoleManager.h"

extern int Global_WorldHexRingsNum;
extern float Global_WorldHexSize;
extern const float Global_NetCullDistanceHexRepActor;
extern const uint32 Global_RepActorsNumPerHex;

extern bool Global_DebugEnableFlushForDormantActor;

extern TAutoConsoleVariable<bool> CVar_DebugShowInfoForHexRepActor;

//extern FAutoConsoleCommand CCmdHexEnableFlushForDormantActor;

DECLARE_LOG_CATEGORY_EXTERN( LogHexRepGraph, Log, All );
