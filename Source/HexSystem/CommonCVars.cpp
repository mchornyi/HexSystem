#include "CommonCVars.h"

#include "HexSystemCharacter.h"
#include "HexWorld.h"

DEFINE_LOG_CATEGORY(LogHexRepGraph);
DEFINE_LOG_CATEGORY(LogHexSystem);

int GWorldHexRingsNum = 1;
float GWorldHexSize = 200.0f;
const float GNetCullDistanceHexRepActor = 1000.0f;
const uint32 GRepActorsNumPerHex = 10000;

TAutoConsoleVariable<bool> GCVar_DebugShowInfoForHexRepActor(TEXT("hex.Debug.ShowInfoForHexRepActor"), true, TEXT("Show debug info of AHexReplicatorDebugActor."), ECVF_Default);

static FAutoConsoleCommand CmdHexGenerateRepActors = FAutoConsoleCommand(TEXT("hex.Cmd.GenerateRepActors"),
                                                                         TEXT("Generates a lot of HexReplicatorDebugActor to test CPU performance for network."),
                                                                         FConsoleCommandDelegate::CreateLambda(
                                                                             []()
                                                                             {
                                                                                 for(TObjectIterator<AHexWorld> itr; itr; ++itr)
                                                                                     (*itr)->DebugGenerateRepActors();
                                                                             }));

static FAutoConsoleCommand CmdHexServerRPCFlushOnce = FAutoConsoleCommand(TEXT("hex.Cmd.ServerRPCFlushOnce"),
                                                                          TEXT("Sends a command to the server to flush a dormant actor only once"),
                                                                          FConsoleCommandDelegate::CreateLambda(
                                                                              []()
                                                                              {
                                                                                  for(TObjectIterator<AHexSystemCharacter> itr; itr; ++itr)
                                                                                      (*itr)->ServerRPCFlushOnce();
                                                                              }));
