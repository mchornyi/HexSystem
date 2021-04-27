#include "CommonCVars.h"

//TAutoConsoleVariable<int> CVar_WorldHexRingsNum(
//    TEXT( "hex.WorldHexRingsNum" ),
//    6,
//    TEXT( "Defines the number of hex rings." ),
//    ECVF_Default );
//
//TAutoConsoleVariable<float> CVar_WorldHexSize(
//    TEXT( "hex.WorldHexSize" ),
//    100.0f,
//    TEXT( "Defines the size of hex cell." ),
//    ECVF_Default );

DEFINE_LOG_CATEGORY( LogHexRepGraph );

int Global_WorldHexRingsNum = 1;
float Global_WorldHexSize = 200.0f;
const float Global_NetCullDistanceHexRepActor = 1000.0f;
const uint32 Global_RepActorsNumPerHex = 10000;
bool Global_DebugEnableFlushingForDormantActor = false;

TAutoConsoleVariable<bool> CVar_DebugShowInfoForHexRepActor(
    TEXT( "hex.Debug.ShowInfoForHexRepActor" ),
    true,
    TEXT( "Show debug info of AHexReplicatorDebugActor." ),
    ECVF_Default );

TAutoConsoleVariable<bool> CVar_DebugEnableFlushingForDormantActor(
    TEXT( "hex.Debug.EnableFlushingForDormantActor" ),
    false,
    TEXT( "Enable flushing of AHexReplicatorDebugDormantActor." ),
    ECVF_Default );