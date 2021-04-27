#pragma once

#include "HAL/IConsoleManager.h"

extern int Global_WorldHexRingsNum;
extern float Global_WorldHexSize;
extern const float Global_NetCullDistanceHexRepActor;
extern const uint32 Global_RepActorsNumPerHex;

extern bool Global_DebugEnableFlushingForDormantActor;

extern TAutoConsoleVariable<bool> CVar_DebugShowInfoForHexRepActor;

extern TAutoConsoleVariable<bool> CVar_DebugEnableFlushingForDormantActor;

DECLARE_LOG_CATEGORY_EXTERN( LogHexRepGraph, Log, All );
