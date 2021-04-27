#include "HexDormancyNode.h"
#include "../CommonCVars.h"

static int32 CVar_RepGraph_DormancyNode_DisconnectedBehavior = 1;
static FAutoConsoleVariableRef CVarRepGraphDormancyNodeDisconnectedBehavior(
    TEXT("hex.RepGraph.DormancyNodeDisconnectedBehavior"), CVar_RepGraph_DormancyNode_DisconnectedBehavior, TEXT(
        "This changes how the dormancy node deals with disconnected clients. 0 = ignore. 1 = skip the disconnected client nodes. 2 = lazily destroy the disconnected client nodes"),
    ECVF_Default);

UReplicationGraphNode_HexConnectionDormancyNode::UReplicationGraphNode_HexConnectionDormancyNode()
{
    FlushedActorList.PrepareForWrite();
}

void UReplicationGraphNode_HexConnectionDormancyNode::GatherActorListsForConnection(
    const FConnectionGatherActorListParameters& Params)
{
    ConditionalGatherDormantActorsForConnection(ReplicationActorList, Params, nullptr);

    for(int32 idx = StreamingLevelCollection.StreamingLevelLists.Num() - 1; idx >= 0; --idx)
    {
        FStreamingLevelActorListCollection::FStreamingLevelActors& StreamingList = StreamingLevelCollection.
            StreamingLevelLists[idx];
        if(StreamingList.ReplicationActorList.Num() <= 0)
        {
            StreamingLevelCollection.StreamingLevelLists.RemoveAtSwap(idx, 1, false);
            continue;
        }

        if(Params.CheckClientVisibilityForLevel(StreamingList.StreamingLevelName))
        {
            FHexStreamingLevelActorListCollection::FStreamingLevelActors* RemoveList =
                RemovedStreamingLevelActorListCollection.StreamingLevelLists.
                                                         FindByKey(StreamingList.StreamingLevelName);
            if(!RemoveList)
            {
                RemoveList = &RemovedStreamingLevelActorListCollection.StreamingLevelLists.Emplace_GetRef(
                    StreamingList.StreamingLevelName);
                Params.ConnectionManager.OnClientVisibleLevelNameAddMap.FindOrAdd(StreamingList.StreamingLevelName).
                       AddUObject(this, &UReplicationGraphNode_HexConnectionDormancyNode::OnClientVisibleLevelNameAdd);
            }

            ConditionalGatherDormantActorsForConnection(StreamingList.ReplicationActorList, Params,
                                                        &RemoveList->ReplicationActorList);
        }
        else
        {
            UE_LOG(LogHexRepGraph, Verbose, TEXT( "Level Not Loaded %s. (Client has %d levels loaded)" ),
                   *StreamingList.StreamingLevelName.ToString( ), Params.ClientVisibleLevelNamesRef.Num( ));
        }
    }
}

void UReplicationGraphNode_HexConnectionDormancyNode::ConditionalGatherDormantActorsForConnection(
    FActorRepListRefView& DormantActorList, const FConnectionGatherActorListParameters& Params,
    FActorRepListRefView* RemovedList)
{
    //FPerConnectionActorInfoMap& ConnectionActorInfoMap = Params.ConnectionManager.ActorInfoMap;
    //FGlobalActorReplicationInfoMap* GlobalActorReplicationInfoMap = GraphGlobals->GlobalActorReplicationInfoMap;

    //FlushedActorList.Reset();

    // We can trickle if the TrickelStartCounter is 0. (Just trying to give it a few frames to settle)
    //bool bShouldTrickle = TrickleStartCounter == 0;
    //for(int32 idx = DormantActorList.Num() - 1; idx >= 0; --idx)
    //{
    //    FActorRepListType Actor = DormantActorList[idx];
    //    FConnectionReplicationActorInfo& ConnectionActorInfo = ConnectionActorInfoMap.FindOrAdd(Actor);

    //    if(!ConnectionActorInfo.bDormantOnConnection)
    //    {
    //        FlushedActorList.Add(Actor);

    //        // If we trickled this actor, restore CullDistance to the default
    //        //if ( ConnectionActorInfo.GetCullDistanceSquared( ) <= 0.f )
    //        //{
    //        //	FGlobalActorReplicationInfo& GlobalInfo = GlobalActorReplicationInfoMap->Get( Actor );
    //        //	ConnectionActorInfo.SetCullDistanceSquared( GlobalInfo.Settings.GetCullDistanceSquared( ) );
    //        //}

    //        // He can be removed
    //        //if ( RemovedList )
    //        //{
    //        //	RemovedList->PrepareForWrite( );
    //        //	RemovedList->Add( Actor );
    //        //}

    //        //UE_CLOG( CVar_RepGraph_LogNetDormancyDetails > 0, LogHexRepGraph, Display, TEXT( "GRAPH_DORMANCY: Actor %s is Dormant on %s. Removing from list. (%d elements left)" ), *Actor->GetPathName( ), *GetName( ), DormantActorList.Num( ) );
    //        //bShouldTrickle = false; // Dont trickle this frame because we are still encountering dormant actors
    //    }
    //    //else if ( CVar_RepGraph_TrickleDistCullOnDormancyNodes > 0 && bShouldTrickle )
    //    //{
    //    //	ConnectionActorInfo.SetCullDistanceSquared( 0.f );
    //    //	bShouldTrickle = false; // trickle one actor per frame
    //    //}


    //    DormantActorList.RemoveAtSwap( idx );
    //}

    if(DormantActorList.Num() > 0)
    {
        Params.OutGatheredReplicationLists.AddReplicationActorList(DormantActorList);

        if(TrickleStartCounter > 0)
        {
            TrickleStartCounter--;
        }
    }
}

bool ContainsReverse(const FActorRepListRefView& List, FActorRepListType Actor)
{
    for(int32 idx = List.Num() - 1; idx >= 0; --idx)
    {
        if(List[idx] == Actor)
        {
            return true;
        }
    }

    return false;
}

void UReplicationGraphNode_HexConnectionDormancyNode::NotifyActorDormancyFlush(FActorRepListType Actor)
{
    QUICK_SCOPE_CYCLE_COUNTER(ConnectionDormancyNode_NotifyActorDormancyFlush);

    FNewReplicatedActorInfo ActorInfo(Actor);

    // Dormancy is flushed so we need to make sure this actor is on this connection specific node.
    // Guard against dupes in the list. Sometimes actors flush multiple times in a row or back to back frames.
    //
    // It may be better to track last flush frame on GlobalActorRepInfo?
    if(ActorInfo.StreamingLevelName == NAME_None)
    {
        if(!ContainsReverse(ReplicationActorList, Actor))
        {
            ReplicationActorList.Add(ActorInfo.Actor);
        }
    }
    else
    {
        FStreamingLevelActorListCollection::FStreamingLevelActors* Item = StreamingLevelCollection.StreamingLevelLists.
            FindByKey(ActorInfo.StreamingLevelName);
        if(!Item)
        {
            Item = &StreamingLevelCollection.StreamingLevelLists.Emplace_GetRef(ActorInfo.StreamingLevelName);
            Item->ReplicationActorList.Add(ActorInfo.Actor);
        }
        else if(!ContainsReverse(Item->ReplicationActorList, Actor))
        {
            Item->ReplicationActorList.Add(ActorInfo.Actor);
        }

        // Remove from RemoveList
        FHexStreamingLevelActorListCollection::FStreamingLevelActors* RemoveList =
            RemovedStreamingLevelActorListCollection.StreamingLevelLists.FindByKey(ActorInfo.StreamingLevelName);
        if(RemoveList)
        {
            RemoveList->ReplicationActorList.PrepareForWrite();
            RemoveList->ReplicationActorList.RemoveFast(Actor);
        }
    }
}

void UReplicationGraphNode_HexConnectionDormancyNode::OnClientVisibleLevelNameAdd(FName LevelName, UWorld* World)
{
    FHexStreamingLevelActorListCollection::FStreamingLevelActors* RemoveList = RemovedStreamingLevelActorListCollection.
                                                                               StreamingLevelLists.FindByKey(LevelName);
    if(!RemoveList)
    {
        UE_LOG(LogHexRepGraph, Warning,
               TEXT(
                   ":OnClientVisibleLevelNameAdd called on %s but there is no RemoveList. How did this get bound in the first place?. Level: %s"
               ), *GetPathName( ), *LevelName.ToString( ));
        return;
    }

    FStreamingLevelActorListCollection::FStreamingLevelActors* AddList = StreamingLevelCollection.StreamingLevelLists.
        FindByKey(LevelName);
    if(!AddList)
    {
        AddList = &StreamingLevelCollection.StreamingLevelLists.Emplace_GetRef(LevelName);
    }

    //UE_CLOG( CVar_RepGraph_LogNetDormancyDetails, LogHexRepGraph, Display, TEXT( "::OnClientVisibleLevelNameadd %s. LevelName: %s." ), *GetPathName( ), *LevelName.ToString( ) );
    //UE_CLOG( CVar_RepGraph_LogNetDormancyDetails, LogHexRepGraph, Display, TEXT( "    CurrentAddList: %s" ), *AddList->ReplicationActorList.BuildDebugString( ) );
    //UE_CLOG( CVar_RepGraph_LogNetDormancyDetails, LogHexRepGraph, Display, TEXT( "    RemoveList: %s" ), *RemoveList->ReplicationActorList.BuildDebugString( ) );

    AddList->ReplicationActorList.PrepareForWrite();
    AddList->ReplicationActorList.AppendContentsFrom(RemoveList->ReplicationActorList);

    RemoveList->ReplicationActorList.PrepareForWrite();
    RemoveList->ReplicationActorList.Reset();
}

bool UReplicationGraphNode_HexConnectionDormancyNode::NotifyRemoveNetworkActor(
    const FNewReplicatedActorInfo& ActorInfo, bool WarnIfNotFound)
{
    QUICK_SCOPE_CYCLE_COUNTER(ConnectionDormancyNode_NotifyRemoveNetworkActor);

    // Remove from active list by calling super
    if(Super::RemoveNetworkActorFast(ActorInfo))
    {
        return true;
    }

    // Not found in active list. We must check out RemovedActorList
    return RemovedStreamingLevelActorListCollection.RemoveActorFast(ActorInfo, this);
}

void UReplicationGraphNode_HexConnectionDormancyNode::NotifyResetAllNetworkActors()
{
    Super::NotifyResetAllNetworkActors();
    RemovedStreamingLevelActorListCollection.Reset();
}

void UReplicationGraphNode_HexConnectionDormancyNode::OnPostReplicatePrioritizeLists(
    UNetReplicationGraphConnection*, FPrioritizedRepList*)
{
    ReplicationActorList.Reset();
}

// --------------------------------------------------------------------------------------------------------------------------------------------
#pragma region UReplicationGraphNode_HexDormancyNode
float UReplicationGraphNode_HexDormancyNode::MaxZForConnection = WORLD_MAX;

void UReplicationGraphNode_HexDormancyNode::CallFunctionOnValidConnectionNodes(FConnectionDormancyNodeFunction Function)
{
    enum class DisconnectedClientNodeBehavior
    {
        AlwaysValid = 0,
        // Keep calling functions on disconnected client nodes (previous behavior)
        Deactivate = 1,
        // Deactivate the nodes by ignoring them. (wastes memory but doesn't incur a costly destruction)
        Destroy = 2,
        // Destroy the nodes immediately (one time cpu hit)
    };
    const DisconnectedClientNodeBehavior DisconnectedClientBehavior = (DisconnectedClientNodeBehavior)
        CVar_RepGraph_DormancyNode_DisconnectedBehavior;

    QUICK_SCOPE_CYCLE_COUNTER(UReplicationGraphNode_HexDormancyNode_ConnectionLoop);
    for(FConnectionDormancyNodeMap::TIterator It = ConnectionNodes.CreateIterator(); It; ++It)
    {
        const FRepGraphConnectionKey RepGraphConnection = It.Key();
        const bool bIsActiveConnection = (DisconnectedClientBehavior == DisconnectedClientNodeBehavior::AlwaysValid) ||
            (RepGraphConnection.ResolveObjectPtr() != nullptr);
        if(bIsActiveConnection)
        {
            Function(It.Value());
        }
        else if(DisconnectedClientBehavior == DisconnectedClientNodeBehavior::Destroy)
        {
            // The connection is now invalid. Destroy it's corresponding node
            UReplicationGraphNode_HexConnectionDormancyNode* ConnectionNodeToDestroy = It.Value();

            bool bWasRemoved = RemoveChildNode(ConnectionNodeToDestroy,
                                               UReplicationGraphNode::NodeOrdering::IgnoreOrdering);
            ensureMsgf(bWasRemoved, TEXT( "DormancyNode did not find %s in it's child node." ),
                       *ConnectionNodeToDestroy->GetName( ));

            //TODO: Release the ActorList in the Node ?

            It.RemoveCurrent();
        }
    }
}

void UReplicationGraphNode_HexDormancyNode::NotifyResetAllNetworkActors()
{
    if(GraphGlobals.IsValid())
    {
        // Unregister dormancy callbacks first
        for(FActorRepListType& Actor : ReplicationActorList)
        {
            FGlobalActorReplicationInfo& GlobalInfo = GraphGlobals->GlobalActorReplicationInfoMap->Get(Actor);
            GlobalInfo.Events.DormancyFlush.RemoveAll(this);
        }
    }

    // Dump our global actor list
    Super::NotifyResetAllNetworkActors();

    auto ResetAllActorsFunction = [ ](UReplicationGraphNode_HexConnectionDormancyNode* ConnectionNode)
    {
        ConnectionNode->NotifyResetAllNetworkActors();
    };
    CallFunctionOnValidConnectionNodes(ResetAllActorsFunction);
}

void UReplicationGraphNode_HexDormancyNode::AddDormantActor(const FNewReplicatedActorInfo& ActorInfo,
                                                            FGlobalActorReplicationInfo& GlobalInfo)
{
    QUICK_SCOPE_CYCLE_COUNTER(DormancyNode_AddDormantActor);

    Super::NotifyAddNetworkActor(ActorInfo);

    //UE_CLOG( CVar_RepGraph_LogNetDormancyDetails > 0 && ConnectionNodes.Num( ) > 0, LogHexRepGraph, Display, TEXT( "GRAPH_DORMANCY: AddDormantActor %s on %s. Adding to %d connection nodes." ), *ActorInfo.Actor->GetPathName( ), *GetName( ), ConnectionNodes.Num( ) );

    auto AddActorFunction = [ ActorInfo ](UReplicationGraphNode_HexConnectionDormancyNode* ConnectionNode)
    {
        QUICK_SCOPE_CYCLE_COUNTER(ConnectionDormancyNode_NotifyAddNetworkActor);
        ConnectionNode->NotifyAddNetworkActor(ActorInfo);
    };
    CallFunctionOnValidConnectionNodes(AddActorFunction);

    // Tell us if this guy flushes net dormancy so we force him back on connection lists
    GlobalInfo.Events.DormancyFlush.AddUObject(this, &UReplicationGraphNode_HexDormancyNode::OnActorDormancyFlush);
}

void UReplicationGraphNode_HexDormancyNode::RemoveDormantActor(const FNewReplicatedActorInfo& ActorInfo,
                                                               FGlobalActorReplicationInfo& ActorRepInfo)
{
    QUICK_SCOPE_CYCLE_COUNTER(DormancyNode_RemoveDormantActor);

    //UE_CLOG( CVar_RepGraph_LogActorRemove > 0, LogHexRepGraph, Display, TEXT( "UReplicationGraphNode_HexDormancyNode::RemoveDormantActor %s on %s. (%d connection nodes). ChildNodes: %d" ), *GetNameSafe( ActorInfo.Actor ), *GetPathName( ), ConnectionNodes.Num( ), AllChildNodes.Num( ) );

    Super::RemoveNetworkActorFast(ActorInfo);

    ActorRepInfo.Events.DormancyFlush.RemoveAll(this);

    auto RemoveActorFunction = [ ActorInfo ](UReplicationGraphNode_HexConnectionDormancyNode* ConnectionNode)
    {
        // Don't warn if not found, the node may have removed the actor itself. Not worth the extra bookkeeping to skip the call.
        ConnectionNode->NotifyRemoveNetworkActor(ActorInfo, false);
    };
    CallFunctionOnValidConnectionNodes(RemoveActorFunction);
}

void UReplicationGraphNode_HexDormancyNode::GatherActorListsForConnection(
    const FConnectionGatherActorListParameters& Params)
{
    int32 NumViewersAboveMaxZ = 0;
    for(const FNetViewer& CurViewer : Params.Viewers)
    {
        if(CurViewer.ViewLocation.Z > MaxZForConnection)
        {
            ++NumViewersAboveMaxZ;
        }
    }

    // If we're above max on all viewers, don't gather actors.
    if(Params.Viewers.Num() <= NumViewersAboveMaxZ)
    {
        return;
    }

    UReplicationGraphNode_HexConnectionDormancyNode* ConnectionNode = GetConnectionNode(Params);
    ConnectionNode->GatherActorListsForConnection(Params);
}

UReplicationGraphNode_HexConnectionDormancyNode* UReplicationGraphNode_HexDormancyNode::GetExistingConnectionNode(
    const FConnectionGatherActorListParameters& Params)
{
    UReplicationGraphNode_HexConnectionDormancyNode** ConnectionNodeItem = ConnectionNodes.Find(
        FRepGraphConnectionKey(&Params.ConnectionManager));
    return ConnectionNodeItem == nullptr ? nullptr : *ConnectionNodeItem;
}

UReplicationGraphNode_HexConnectionDormancyNode* UReplicationGraphNode_HexDormancyNode::GetConnectionNode(
    const FConnectionGatherActorListParameters& Params)
{
    FRepGraphConnectionKey RepGraphConnection(&Params.ConnectionManager);
    UReplicationGraphNode_HexConnectionDormancyNode** NodePtrPtr = ConnectionNodes.Find(RepGraphConnection);
    UReplicationGraphNode_HexConnectionDormancyNode* ConnectionNode = NodePtrPtr != nullptr ? *NodePtrPtr : nullptr;

    if(ConnectionNode == nullptr)
    {
        // We dont have a per-connection node for this connection, so create one and copy over contents
        ConnectionNode = CreateChildNode<UReplicationGraphNode_HexConnectionDormancyNode>();
        ConnectionNodes.Add(RepGraphConnection) = ConnectionNode;

        // Copy our master lists to the connection node
        //ConnectionNode->DeepCopyActorListsFrom( this );

        Params.ConnectionManager.OnPostReplicatePrioritizeLists.AddUObject( ConnectionNode, &UReplicationGraphNode_HexConnectionDormancyNode::OnPostReplicatePrioritizeLists );

        //UE_CLOG( CVar_RepGraph_LogNetDormancyDetails > 0, LogHexRepGraph, Display, TEXT( "GRAPH_DORMANCY: First time seeing connection %s in node %s. Created ConnectionDormancyNode %s." ), *Params.ConnectionManager.GetName( ), *GetName( ), *ConnectionNode->GetName( ) );
    }

    return ConnectionNode;
}

void UReplicationGraphNode_HexDormancyNode::OnActorDormancyFlush(FActorRepListType Actor,
                                                                 FGlobalActorReplicationInfo& GlobalInfo)
{
    QUICK_SCOPE_CYCLE_COUNTER(DormancyNode_OnActorDormancyFlush);

    //if ( CVar_RepGraph_Verify )
    //{
    //	FNewReplicatedActorInfo ActorInfo( Actor );
    //	if ( ActorInfo.StreamingLevelName == NAME_None )
    //	{
    //		ensureMsgf( ReplicationActorList.Contains( Actor ), TEXT( "UReplicationGraphNode_HexDormancyNode::OnActorDormancyFlush %s not present in %s actor lists!" ), *Actor->GetPathName( ), *GetPathName( ) );
    //	}
    //	else
    //	{
    //		if ( FHexStreamingLevelActorListCollection::FStreamingLevelActors* Item = StreamingLevelCollection.StreamingLevelLists.FindByKey( ActorInfo.StreamingLevelName ) )
    //		{
    //			ensureMsgf( Item->ReplicationActorList.Contains( Actor ), TEXT( "UReplicationGraphNode_HexDormancyNode::OnActorDormancyFlush %s not present in %s actor lists! Streaming Level: %s" ), *GetActorRepListTypeDebugString( Actor ), *GetPathName( ), *ActorInfo.StreamingLevelName.ToString( ) );
    //		}
    //	}
    //}

    // -------------------

    //UE_CLOG( CVar_RepGraph_LogNetDormancyDetails > 0 && ConnectionNodes.Num( ) > 0, LogHexRepGraph, Display, TEXT( "GRAPH_DORMANCY: Actor %s Flushed Dormancy. %s. Refreshing all %d connection nodes." ), *Actor->GetPathName( ), *GetName( ), ConnectionNodes.Num( ) );

    auto DormancyFlushFunction = [ Actor ](UReplicationGraphNode_HexConnectionDormancyNode* ConnectionNode)
    {
        ConnectionNode->NotifyActorDormancyFlush(Actor);
    };
    CallFunctionOnValidConnectionNodes(DormancyFlushFunction);
}

void UReplicationGraphNode_HexDormancyNode::ConditionalGatherDormantDynamicActors(
    FActorRepListRefView& RepList, const FConnectionGatherActorListParameters& Params,
    FActorRepListRefView* RemovedList, bool bEnforceReplistUniqueness)
{
    for(FActorRepListType& Actor : ReplicationActorList)
    {
        if(Actor && !Actor->IsNetStartupActor())
        {
            if(FConnectionReplicationActorInfo* Info = Params.ConnectionManager.ActorInfoMap.Find(Actor))
            {
                if(Info->bDormantOnConnection)
                {
                    if(RemovedList && RemovedList->IsValid() && RemovedList->Contains(Actor))
                    {
                        continue;
                    }

                    // Prevent adding actors if we already have added them, this saves on grow operations.
                    if(bEnforceReplistUniqueness)
                    {
                        if(Info->bGridSpatilization_AlreadyDormant)
                        {
                            continue;
                        }
                        else
                        {
                            Info->bGridSpatilization_AlreadyDormant = true;
                        }
                    }

                    RepList.PrepareForWrite();
                    RepList.ConditionalAdd(Actor);
                }
            }
        }
    }
}

#pragma endregion UReplicationGraphNode_HexDormancyNode

#pragma region FHexStreamingLevelActorListCollection
// --------------------------------------------------------------------------------------------------------------------------------------------
void FHexStreamingLevelActorListCollection::AddActor(const FNewReplicatedActorInfo& ActorInfo)
{
    FStreamingLevelActors* Item = StreamingLevelLists.FindByKey(ActorInfo.StreamingLevelName);
    if(!Item)
    {
        Item = &StreamingLevelLists.Emplace_GetRef(ActorInfo.StreamingLevelName);
    }

    if(CVar_RepGraph_Verify)
    {
        ensureMsgf(Item->ReplicationActorList.Contains( ActorInfo.Actor ) == false,
                   TEXT( "%s being added to %s twice! Streaming level: %s" ),
                   *GetActorRepListTypeDebugString( ActorInfo.Actor ), *ActorInfo.StreamingLevelName.ToString( ));
    }

    Item->ReplicationActorList.Add(ActorInfo.Actor);
}

bool FHexStreamingLevelActorListCollection::RemoveActor(const FNewReplicatedActorInfo& ActorInfo, bool bWarnIfNotFound,
                                                        UReplicationGraphNode* Outer)
{
    bool bRemovedSomething = false;
    for(FStreamingLevelActors& StreamingList : StreamingLevelLists)
    {
        if(StreamingList.StreamingLevelName == ActorInfo.StreamingLevelName)
        {
            bRemovedSomething = StreamingList.ReplicationActorList.Remove(ActorInfo.Actor);
            if(!bRemovedSomething && bWarnIfNotFound)
            {
                UE_LOG(LogHexRepGraph, Warning,
                       TEXT( "Attempted to remove %s from list %s but it was not found. (StreamingLevelName == %s)" ),
                       *GetActorRepListTypeDebugString( ActorInfo.Actor ), *GetPathNameSafe( Outer ),
                       *ActorInfo.StreamingLevelName.ToString( ));
            }

            if(CVar_RepGraph_Verify)
            {
                ensureMsgf(StreamingList.ReplicationActorList.Contains( ActorInfo.Actor ) == false,
                           TEXT( "Actor %s is still in %s after removal. Streaming Level: %s" ),
                           *GetActorRepListTypeDebugString( ActorInfo.Actor ), *GetPathNameSafe( Outer ));
            }
            break;
        }
    }
    return bRemovedSomething;
}

bool FHexStreamingLevelActorListCollection::RemoveActorFast(const FNewReplicatedActorInfo& ActorInfo,
                                                            UReplicationGraphNode* Outer)
{
    bool bRemovedSomething = false;
    for(FStreamingLevelActors& StreamingList : StreamingLevelLists)
    {
        if(StreamingList.StreamingLevelName == ActorInfo.StreamingLevelName)
        {
            bRemovedSomething = StreamingList.ReplicationActorList.RemoveFast(ActorInfo.Actor);
            break;
        }
    }
    return bRemovedSomething;
}

void FHexStreamingLevelActorListCollection::Reset()
{
    for(FStreamingLevelActors& StreamingList : StreamingLevelLists)
    {
        StreamingList.ReplicationActorList.Reset();
    }
}

void FHexStreamingLevelActorListCollection::Gather(const FConnectionGatherActorListParameters& Params)
{
    for(const FStreamingLevelActors& StreamingList : StreamingLevelLists)
    {
        if(Params.CheckClientVisibilityForLevel(StreamingList.StreamingLevelName))
        {
            Params.OutGatheredReplicationLists.AddReplicationActorList(StreamingList.ReplicationActorList);
        }
        else
        {
            UE_LOG(LogHexRepGraph, Verbose, TEXT( "Level Not Loaded %s. (Client has %d levels loaded)" ),
                   *StreamingList.StreamingLevelName.ToString( ), Params.ClientVisibleLevelNamesRef.Num( ));
        }
    }
}

void FHexStreamingLevelActorListCollection::DeepCopyFrom(const FHexStreamingLevelActorListCollection& Source)
{
    StreamingLevelLists.Reset();
    for(const FStreamingLevelActors& StreamingLevel : Source.StreamingLevelLists)
    {
        if(StreamingLevel.ReplicationActorList.Num() > 0)
        {
            FStreamingLevelActors& NewStreamingLevel = StreamingLevelLists.Emplace_GetRef(
                StreamingLevel.StreamingLevelName);
            NewStreamingLevel.ReplicationActorList.CopyContentsFrom(StreamingLevel.ReplicationActorList);
            ensure(NewStreamingLevel.ReplicationActorList.Num( ) == StreamingLevel.ReplicationActorList.Num( ));
        }
    }
}

void FHexStreamingLevelActorListCollection::GetAll_Debug(TArray<FActorRepListType>& OutArray) const
{
    for(const FStreamingLevelActors& StreamingLevel : StreamingLevelLists)
    {
        StreamingLevel.ReplicationActorList.AppendToTArray(OutArray);
    }
}

void FHexStreamingLevelActorListCollection::Log(FReplicationGraphDebugInfo& DebugInfo) const
{
    for(const FStreamingLevelActors& StreamingLevelList : StreamingLevelLists)
    {
        LogActorRepList(DebugInfo, StreamingLevelList.StreamingLevelName.ToString(),
                        StreamingLevelList.ReplicationActorList);
    }
}

// --------------------------------------------------------------------------------------------------------------------------------------------
#pragma endregion FHexStreamingLevelActorListCollection
