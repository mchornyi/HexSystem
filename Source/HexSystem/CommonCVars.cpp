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

int Global_WorldHexRingsNum = 1;
float Global_WorldHexSize = 200.0f;
float Global_NetCullDistanceHexRepActor = 1000.0f;

TAutoConsoleVariable<bool> CVar_ShowDebugInfoForHexRepActor(
    TEXT( "hex.HexRepActor.ShowDebugInfo" ),
    true,
    TEXT( "Show debug info of AHexReplicatorDebugActor." ),
    ECVF_Default );