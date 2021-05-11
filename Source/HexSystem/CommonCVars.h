#pragma once

#include "HAL/IConsoleManager.h"

extern int GWorldHexRingsNum;
extern float GWorldHexSize;
extern const float GNetCullDistanceHexRepActor;
extern const uint32 GRepActorsNumPerHex;
extern bool GDebugEnableFlushingForDormantActorInTick;

extern TAutoConsoleVariable<bool> GCVar_DebugShowInfoForHexRepActor;

//extern TAutoConsoleVariable<bool> GCVar_DebugEnableFlushingForDormantActorInTick;

DECLARE_LOG_CATEGORY_EXTERN( LogHexRepGraph, Log, All );
DECLARE_LOG_CATEGORY_EXTERN( LogHexSystem, Log, All );
