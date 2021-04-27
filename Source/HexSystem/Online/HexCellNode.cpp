#include "HexCellNode.h"

#include "../CommonCVars.h"


using namespace hexsystem;

UReplicationGraphNode_HexCell::UReplicationGraphNode_HexCell( )
{
    ReplicationActorList.Reset( Global_RepActorsNumPerHex );
}

void UReplicationGraphNode_HexCell::AddStaticActor( const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& ActorRepInfo, bool bParentNodeHandlesDormancyChange )
{
    //UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    //RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexCell_AddStaticActor );

    if ( ActorRepInfo.bWantsToBeDormant ) // Pass to dormancy node
        GetDormancyNode( )->AddDormantActor( ActorInfo, ActorRepInfo );
    else // Put him in our non dormancy list
        Super::NotifyAddNetworkActor( ActorInfo );

    // We need to be told if this actor changes dormancy so we can move him between nodes. Unless our parent is going to do it.
    if ( !bParentNodeHandlesDormancyChange )
        ActorRepInfo.Events.DormancyChange.AddUObject( this, &UReplicationGraphNode_HexCell::OnStaticActorNetDormancyChange );
}

void UReplicationGraphNode_HexCell::AddDynamicActor( const FNewReplicatedActorInfo& ActorInfo )
{
    //UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    //RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexCell_AddDynamicActor );
    GetDynamicNode( )->NotifyAddNetworkActor( ActorInfo );
}

void UReplicationGraphNode_HexCell::RemoveStaticActor( const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& ActorRepInfo, bool bWasAddedAsDormantActor )
{
    UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexCell_RemoveStaticActor );
    //UE_CLOG( CVar_RepGraph_LogActorRemove > 0, LogReplicationGraph, Display, TEXT( "UReplicationGraphNode_Simple2DSpatializationLeaf::RemoveStaticActor %s on %s" ), *actorInfo.actor->GetPathName( ), *GetPathName( ) );

    if ( bWasAddedAsDormantActor )
        GetDormancyNode( )->RemoveDormantActor( ActorInfo, ActorRepInfo );
    else
        Super::NotifyRemoveNetworkActor( ActorInfo );

    ActorRepInfo.Events.DormancyChange.RemoveAll( this );
}

void UReplicationGraphNode_HexCell::RemoveDynamicActor( const FNewReplicatedActorInfo& actorInfo )
{
    //UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    //RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexCell_RemoveDynamicActor );
    //UE_CLOG( CVar_RepGraph_LogActorRemove > 0, LogReplicationGraph, Display, TEXT( "UReplicationGraphNode_Simple2DSpatializationLeaf::RemoveDynamicActor %s on %s" ), *actorInfo.actor->GetPathName( ), *GetPathName( ) );

    GetDynamicNode( )->NotifyRemoveNetworkActor( actorInfo );
}

void UReplicationGraphNode_HexCell::ConditionalCopyDormantActors( FActorRepListRefView& FromList, UReplicationGraphNode_DormancyNode* ToNode ) const
{
    UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexCell_ConditionalCopyDormantActors );

    if ( !GraphGlobals.IsValid( ) )
        return;

    for ( int32 idx = FromList.Num( ) - 1; idx >= 0; --idx )
    {
        FActorRepListType Actor = FromList[ idx ];
        FGlobalActorReplicationInfo& GlobalInfo = GraphGlobals->GlobalActorReplicationInfoMap->Get( Actor );
        if ( GlobalInfo.bWantsToBeDormant )
        {
            ToNode->NotifyAddNetworkActor( FNewReplicatedActorInfo( Actor ) );
            FromList.RemoveAtSwap( idx );
        }
    }
}

void UReplicationGraphNode_HexCell::OnStaticActorNetDormancyChange( FActorRepListType Actor, FGlobalActorReplicationInfo& GlobalInfo, ENetDormancy NewValue, ENetDormancy OldValue )
{
    UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexCell_OnStaticActorNetDormancyChange );
    //UE_CLOG( CVar_RepGraph_LogNetDormancyDetails > 0, LogReplicationGraph, Display, TEXT( "UReplicationGraphNode_HexCell::OnNetDormancyChange. %s on %s. Old: %d, New: %d" ), *actor->GetPathName( ), *GetPathName( ), NewValue, OldValue );

    const bool bCurrentDormant = NewValue > DORM_Awake;
    const bool bPreviousDormant = OldValue > DORM_Awake;

    if ( !bCurrentDormant && bPreviousDormant )
    {
        // actor is now awake, remove from dormancy node and add to non dormancy list
        const FNewReplicatedActorInfo ActorInfo( Actor );
        GetDormancyNode( )->RemoveDormantActor( ActorInfo, GlobalInfo );
        Super::NotifyAddNetworkActor( ActorInfo );
    }
    else if ( bCurrentDormant && !bPreviousDormant )
    {
        // actor is now dormant, remove from non dormant list, add to dormant node
        const FNewReplicatedActorInfo ActorInfo( Actor );
        Super::NotifyRemoveNetworkActor( ActorInfo );
        GetDormancyNode( )->AddDormantActor( ActorInfo, GlobalInfo );
    }
}

void UReplicationGraphNode_HexCell::GetAllActorsInNode_Debugging( TArray<FActorRepListType>& OutArray ) const
{
    UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexCell_GetAllActorsInNode_Debugging );

    Super::GetAllActorsInNode_Debugging( OutArray );

    if ( DynamicNode )
        DynamicNode->GetAllActorsInNode_Debugging( OutArray );

    if ( DormancyNode )
        DormancyNode->GetAllActorsInNode_Debugging( OutArray );
}