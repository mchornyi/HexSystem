#include "HexReplicationNodes.h"

DECLARE_LOG_CATEGORY_EXTERN( LogHexRepGraph, Log, All );
DEFINE_LOG_CATEGORY( LogHexRepGraph );

using namespace hexsystem;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int32 CVar_HexRepGraph_NbDestroyedGridsToTriggerGC = 100;
static FAutoConsoleVariableRef CVarRepGraphNbDestroyedGridsToTriggerGC(
    TEXT( "Net.HexRepGraph.NbDestroyedGridsToTriggerGC" ),
    CVar_HexRepGraph_NbDestroyedGridsToTriggerGC,
    TEXT( "After destroying this many grids, force a garbage collection to free memory" ),
    ECVF_Default );

int32 CVar_HexRepGraph_DormantDynamicActorsDestruction = 0;
static FAutoConsoleVariableRef CVarRepGraphDormantDynamicActorsDestruction(
    TEXT( "Net.HexRepGraph.DormantDynamicActorsDestruction" ),
    CVar_HexRepGraph_DormantDynamicActorsDestruction,
    TEXT( "If true, irrelevant dormant actors will be destroyed on the client" ), ECVF_Default );

int32 CVar_RepGraph_DebugNextNewActor = 0;
static FAutoConsoleVariableRef CVarRepGraphDebugNextActor(
    TEXT( "Net.HexRepGraph.Spatial.DebugNextNewActor" ),
    CVar_RepGraph_DebugNextNewActor, TEXT( "" ),
    ECVF_Default );

int32 CVar_RepGraph_Spatial_PauseDynamic = 0;
static FAutoConsoleVariableRef CVarRepSpatialPauseDynamic( TEXT( "Net.HexRepGraph.Spatial.PauseDynamic" ), CVar_RepGraph_Spatial_PauseDynamic, TEXT( "Pauses updating dynamic actor positions in the spatialization nodes." ), ECVF_Default );

namespace
{
    FHexLayout sTmpHexLayout( hexsystem::LayoutPointy, {200.0f, 200.0f }, {0.0f, 0.0f });//TODO: Sync between HexWorld
    uint16 sTmpMaxRing = 100;//TODO... Pass here real rings num
    uint16 sTmpMaxHexDistToCenter = 1;//TODO... Pass here real dist num
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UReplicationGraphNode_HexCell::AddStaticActor( const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& ActorRepInfo, bool bParentNodeHandlesDormancyChange )
{
    UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexCell_AddStaticActor );

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

UReplicationGraphNode* UReplicationGraphNode_HexCell::GetDynamicNode( )
{
    //UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    //RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexCell_GetDynamicNode );

    if ( DynamicNode != nullptr )
        return DynamicNode;

    if ( DynamicNode == nullptr )
    {
        if ( CreateDynamicNodeOverride )
            DynamicNode = CreateDynamicNodeOverride( this );
        else
            DynamicNode = CreateChildNode<UReplicationGraphNode_ActorListFrequencyBuckets>( );
    }

    return DynamicNode;
}

UReplicationGraphNode_DormancyNode* UReplicationGraphNode_HexCell::GetDormancyNode( )
{
    UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexCell_GetDormancyNode );
    if ( DormancyNode != nullptr )
        return DormancyNode;

    DormancyNode = CreateChildNode<UReplicationGraphNode_DormancyNode>( );

    return DormancyNode;
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UReplicationGraphNode_HexSpatialization2D::UReplicationGraphNode_HexSpatialization2D( )
{
    bRequiresPrepareForReplicationCall = true;
}


void UReplicationGraphNode_HexSpatialization2D::AddSpatialRebuildBlacklistClass( UClass* Class )
{
    mRebuildSpatialBlacklistMap.Set( Class, true );
}

void UReplicationGraphNode_HexSpatialization2D::NotifyAddNetworkActor( const FNewReplicatedActorInfo& ActorInfo )
{
    ensureAlwaysMsgf( false, TEXT( "UReplicationGraphNode_HexSpatialization2D::NotifyAddNetworkActor should not be called directly" ) );
}

bool UReplicationGraphNode_HexSpatialization2D::NotifyRemoveNetworkActor( const FNewReplicatedActorInfo& ActorInfo, bool bWarnIfNotFound )
{
    ensureAlwaysMsgf( false, TEXT( "UReplicationGraphNode_HexSpatialization2D::NotifyRemoveNetworkActor should not be called directly" ) );
    return false;
}

void UReplicationGraphNode_HexSpatialization2D::AddActor_Dormancy( const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& ActorRepInfo )
{
    UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexSpatialization2D_AddActor_Dormancy );

    //UE_CLOG( CVar_RepGraph_LogActorRemove > 0, LogReplicationGraph, Display, TEXT( "UReplicationGraphNode_HexSpatialization2D::AddActor_Dormancy %s on %s" ), *actorInfo.actor->GetFullName( ), *GetPathName( ) );

    if ( ActorRepInfo.bWantsToBeDormant )
        AddActorInternal_Static( ActorInfo, ActorRepInfo, true );
    else
        AddActorInternal_Dynamic( ActorInfo );

    // Tell us if dormancy changes for this actor because then we need to move it. Note we don't care about Flushing.
    ActorRepInfo.Events.DormancyChange.AddUObject( this, &UReplicationGraphNode_HexSpatialization2D::OnNetDormancyChange );
}

void UReplicationGraphNode_HexSpatialization2D::RemoveActor_Static( const FNewReplicatedActorInfo& ActorInfo )
{
    UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    //UE_CLOG( CVar_RepGraph_LogActorRemove > 0, LogReplicationGraph, Display, TEXT( "UReplicationGraphNode_HexSpatialization2D::RemoveActor_Static %s on %s" ), *actorInfo.actor->GetFullName( ), *GetPathName( ) );

    RemoveActorInternal_Static( ActorInfo );
}

void UReplicationGraphNode_HexSpatialization2D::RemoveActor_Dormancy( const FNewReplicatedActorInfo& ActorInfo )
{
    UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexSpatialization2D_RemoveActor_Dormancy );

    //UE_CLOG( CVar_RepGraph_LogActorRemove > 0, LogReplicationGraph, Display, TEXT( "UReplicationGraphNode_HexSpatialization2D::RemoveActor_Dormancy %s on %s" ), *actorInfo.actor->GetFullName( ), *GetPathName( ) );

    if ( !GraphGlobals.IsValid( ) )
        return;

    FGlobalActorReplicationInfo& actorRepGlobalInfo = GraphGlobals->GlobalActorReplicationInfoMap->Get( ActorInfo.Actor );

    if ( actorRepGlobalInfo.bWantsToBeDormant )
        RemoveActorInternal_Static( ActorInfo );
    else
        RemoveActorInternal_Dynamic( ActorInfo );

    // AddActorInternal_Static and AddActorInternal_Dynamic will both override actor information if they are called repeatedly.
    // This means that even if AddActor_Dormancy is called multiple times with the same actor, a single call to RemoveActor_Dormancy
    // will completely remove the actor from either the Static or Dynamic list appropriately.
    // Therefore, it should be safe to call RemoveAll and not worry about trying to track individual delegate handles.
    actorRepGlobalInfo.Events.DormancyChange.RemoveAll( this );
}

void UReplicationGraphNode_HexSpatialization2D::AddActorInternal_Dynamic( const FNewReplicatedActorInfo& actorInfo )
{
    UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexSpatialization2D_AddActorInternal_Dynamic );

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if ( actorInfo.Actor->bAlwaysRelevant )
    {
        UE_LOG( LogHexRepGraph, Warning, TEXT( "Always relevant actor being added to spatialized graph node. %s" ), *GetNameSafe( actorInfo.Actor ) );
        return;
    }
#endif

    //UE_CLOG( CVar_RepGraph_LogActorRemove > 0, LogReplicationGraph, Display, TEXT( "UReplicationGraph::AddActorInternal_Dynamic %s" ), *actorInfo.actor->GetFullName( ) );

    mDynamicSpatializedActors.Emplace( actorInfo.Actor, actorInfo );
}

void UReplicationGraphNode_HexSpatialization2D::AddActorInternal_Static( const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& ActorRepInfo, bool bDormancyDriven )
{
    UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexSpatialization2D_AddActorInternal_Static );

    AActor* Actor = ActorInfo.Actor;
    if ( Actor->IsActorInitialized( ) == false )
    {
        // Make sure its not already in the list. This should really not happen but would be very bad if it did. This list should always be small so doing the safety check seems good.
        for ( int32 idx = mPendingStaticSpatializedActors.Num( ) - 1; idx >= 0; --idx )
        {
            if ( mPendingStaticSpatializedActors[ idx ].actor == ActorInfo.Actor )
            {
                ////UE_LOG( LogReplicationGraph, Warning, TEXT( "UReplicationGraphNode_HexSpatialization2D::AddActorInternal_Static was called on %s when it was already in the mPendingStaticSpatializedActors list!" ), *actor->GetPathName( ) );
                return;
            }
        }

        mPendingStaticSpatializedActors.Emplace( ActorInfo.Actor, bDormancyDriven );
        return;
    }

    AddActorInternal_Static_Implementation( ActorInfo, ActorRepInfo, bDormancyDriven );
}

void UReplicationGraphNode_HexSpatialization2D::AddActorInternal_Static_Implementation( const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& ActorRepInfo, bool bDormancyDriven )
{
    UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexSpatialization2D_AddActorInternal_Static_Implementation );

    AActor* Actor = ActorInfo.Actor;
    const FVector Location3D = Actor->GetActorLocation( );
    ActorRepInfo.WorldLocation = Location3D;

    //if ( CVar_RepGraph_LogActorAdd )
    //{
    //	//UE_LOG( LogReplicationGraph, Display, TEXT( "UReplicationGraphNode_HexSpatialization2D::AddActorInternal_Static placing %s into static grid at %s" ), *actor->GetPathName( ), *ActorRepInfo.WorldLocation.ToString( ) );
    //}

    mStaticSpatializedActors.Emplace( Actor, FStaticActorInfo( ActorInfo, bDormancyDriven ) );

    AddStaticActorIntoCell( ActorInfo, ActorRepInfo, bDormancyDriven );
}

void UReplicationGraphNode_HexSpatialization2D::RemoveActorInternal_Dynamic( const FNewReplicatedActorInfo& actorInfo )
{
    UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexSpatialization2D_RemoveActorInternal_Dynamic );

    FDynamicActorInfo* info = mDynamicSpatializedActors.Find( actorInfo.Actor );
    if ( !info )
    {
        UE_LOG( LogHexRepGraph, Warning, TEXT( "RemoveActorInternal_Dynamic attempted remove %s from streaming dynamic list but it was not there." ), *GetActorRepListTypeDebugString( actorInfo.Actor ) );
        return;
    }

    //TODO: Optimize. Get only those hex that is covered by actor
    for ( auto& hexCell : mHexCells )
        hexCell.Value->RemoveDynamicActor( actorInfo );

    mDynamicSpatializedActors.Remove( actorInfo.Actor );
}

void UReplicationGraphNode_HexSpatialization2D::RemoveActorInternal_Static( const FNewReplicatedActorInfo& actorInfo )
{
    UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexSpatialization2D_RemoveActorInternal_Static );

    if ( !GraphGlobals.IsValid( ) )
        return;

    if ( mStaticSpatializedActors.Remove( actorInfo.Actor ) <= 0 )
    {
        // May have been a pending actor
        for ( int32 idx = mPendingStaticSpatializedActors.Num( ) - 1; idx >= 0; --idx )
        {
            if ( mPendingStaticSpatializedActors[ idx ].actor == actorInfo.Actor )
            {
                mPendingStaticSpatializedActors.RemoveAtSwap( idx, 1, false );
                return;
            }
        }

        UE_LOG( LogHexRepGraph, Warning, TEXT( "RemoveActorInternal_Static attempted remove %s from static list but it was not there." ), *GetActorRepListTypeDebugString( actorInfo.Actor ) );
    }

    FGlobalActorReplicationInfo& actorRepGlobalInfo = GraphGlobals->GlobalActorReplicationInfoMap->Get( actorInfo.Actor );

    // Remove it from the actual node it should still be in. Note that even if the actor did move in between this and the last replication frame, the FGlobalActorReplicationInfo would not have been updated
    for ( auto& hexCell : mHexCells )
        hexCell.Value->RemoveStaticActor( actorInfo, actorRepGlobalInfo, actorRepGlobalInfo.bWantsToBeDormant );

    if ( CVar_RepGraph_Verify )
        RepGraphVerifySlow( actorInfo.Actor, actorRepGlobalInfo, false );
}

void UReplicationGraphNode_HexSpatialization2D::OnNetDormancyChange( FActorRepListType Actor, FGlobalActorReplicationInfo& GlobalInfo, ENetDormancy NewValue, ENetDormancy OldValue )
{
    UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexSpatialization2D_OnNetDormancyChange );

    const bool bCurrentShouldBeStatic = NewValue > DORM_Awake;
    const bool bPreviousShouldBeStatic = OldValue > DORM_Awake;

    if ( bCurrentShouldBeStatic && !bPreviousShouldBeStatic )
    {
        // actor was dynamic and is now static. Remove from dynamic list and add to static.
        FNewReplicatedActorInfo actorInfo( Actor );

        RemoveActorInternal_Dynamic( actorInfo );

        GlobalInfo.bWantsToBeDormant = true;
        AddActorInternal_Static( actorInfo, GlobalInfo, true );
    }
    else if ( !bCurrentShouldBeStatic && bPreviousShouldBeStatic )
    {
        const FNewReplicatedActorInfo actorInfo( Actor );

        GlobalInfo.bWantsToBeDormant = true;

        RemoveActorInternal_Static( actorInfo );

        AddActorInternal_Dynamic( actorInfo );
    }
}

void UReplicationGraphNode_HexSpatialization2D::NotifyResetAllNetworkActors( )
{
    UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    mStaticSpatializedActors.Reset( );
    mDynamicSpatializedActors.Reset( );
    Super::NotifyResetAllNetworkActors( );
}

void UReplicationGraphNode_HexSpatialization2D::AddStaticActorIntoCell( const FNewReplicatedActorInfo& actorInfo, FGlobalActorReplicationInfo& actorRepInfo, bool bDormancyDriven )
{
    UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexSpatialization2D_AddStaticActorIntoCell );

    auto hexCells = GetHexCellCoverage( actorRepInfo.WorldLocation, actorRepInfo.Settings.GetCullDistance( ) );

    for ( auto hexCell : hexCells )
        hexCell->AddStaticActor( actorInfo, actorRepInfo, bDormancyDriven );
}

void UReplicationGraphNode_HexSpatialization2D::PrepareForReplication( )
{
    //UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexSpatialization2D_PrepareForReplication );

    FGlobalActorReplicationInfoMap* globalRepMap = GraphGlobals.IsValid( ) ? GraphGlobals->GlobalActorReplicationInfoMap : nullptr;
    if ( !globalRepMap )
    {
        UE_LOG( LogHexRepGraph, Error, TEXT( "UReplicationGraphNode_HexSpatialization2D::PrepareForReplication: There is no GraphGlobals." ) );
        return;
    }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

    // -------------------------------------------
    //	Update dynamic actors
    // -------------------------------------------
    if ( CVar_RepGraph_Spatial_PauseDynamic == 0 )
#endif
    {
        RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexSpatialization2D_BuildDynamic );

        for ( auto& it : mDynamicSpatializedActors )
        {
            FActorRepListType& dynamicActor = it.Key;
            FDynamicActorInfo& dynamicActorInfo = it.Value;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
            if ( !IsActorValidForReplicationGather( dynamicActor ) )
            {
                UE_LOG( LogHexRepGraph, Warning, TEXT( "UReplicationGraphNode_HexSpatialization2D::PrepareForReplication: Dynamic actor no longer ready for replication" ) );
                UE_LOG( LogHexRepGraph, Warning, TEXT( "%s" ), *GetNameSafe( dynamicActor ) );
                continue;
            }
#endif

            // Update location
            FGlobalActorReplicationInfo& actorRepGlobalInfo = globalRepMap->Get( dynamicActor );

            // Check if this resets spatial bias
            const FVector actorLocationCurrent = dynamicActor->GetActorLocation( );
            const FVector actorLocationPrev = actorRepGlobalInfo.WorldLocation;
            actorRepGlobalInfo.WorldLocation = actorLocationCurrent;

            auto hexCells = GetHexCellCoverage( actorLocationPrev, actorRepGlobalInfo.Settings.GetCullDistance( ) );
            for ( auto hexCell : hexCells )
                hexCell->RemoveDynamicActor( dynamicActorInfo.actorInfo );

            hexCells = GetHexCellCoverage( actorLocationCurrent, actorRepGlobalInfo.Settings.GetCullDistance( ) );
            for ( auto hexCell : hexCells )
                hexCell->AddDynamicActor( dynamicActorInfo.actorInfo );
        }
    }

    // -------------------------------------------
    //	Pending Spatial Actors
    // -------------------------------------------
    for ( int32 idx = mPendingStaticSpatializedActors.Num( ) - 1; idx >= 0; --idx )
    {
        FPendingStaticActors& pendingStaticActor = mPendingStaticSpatializedActors[ idx ];
        if ( pendingStaticActor.actor->IsActorInitialized( ) == false )
            continue;

        FNewReplicatedActorInfo newActorInfo( pendingStaticActor.actor );
        FGlobalActorReplicationInfo& globalInfo = GraphGlobals->GlobalActorReplicationInfoMap->Get( pendingStaticActor.actor );

        AddActorInternal_Static_Implementation( newActorInfo, globalInfo, pendingStaticActor.bDormancyDriven );

        mPendingStaticSpatializedActors.RemoveAtSwap( idx, 1, false );
    }
}

// Small structure to make it easier to keep track of information regarding current players for a connection when working with grids
struct FPlayerHexCellInfo
{
    FPlayerHexCellInfo( )
    {
    }

    FHex curHex;
    FHex prevHex;
};

void UReplicationGraphNode_HexSpatialization2D::GatherActorListsForConnection( const FConnectionGatherActorListParameters& Params )
{
    //UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexSpatialization2D_GatherActorListsForConnection );

    TArray<FLastLocationGatherInfo>& lastLocationArray = Params.ConnectionManager.LastGatherLocations;

    TArray<FHex, FReplicationGraphConnectionsAllocator> uniqueCurrentHexes;

    // Consider all users that are in cells for this connection.
    // From here, generate a list of coordinates, we'll later work through each coordinate pairing
    // to find the cells that are actually active. This reduces redundancy and cache misses.
    TArray<FPlayerHexCellInfo, FReplicationGraphConnectionsAllocator> activeHexCells;

    for ( const FNetViewer& curViewer : Params.Viewers )
    {
        if ( curViewer.ViewLocation.Z > mConnectionMaxZ )
            continue;

        // Figure out positioning
        FVector clampedViewLoc = curViewer.ViewLocation;
        // Prevent extreme locations from causing the mGrid to grow too large
        clampedViewLoc = clampedViewLoc.BoundToCube( HALF_WORLD_MAX );

        FLastLocationGatherInfo* gatherInfoForConnection = nullptr;

        // Save this information out for later.
        if ( curViewer.Connection != nullptr )
        {
            gatherInfoForConnection = lastLocationArray.FindByKey<UNetConnection*>( curViewer.Connection );

            // Add any missing last location information that we don't have
            if ( gatherInfoForConnection == nullptr )
            {
                const auto idx = lastLocationArray.Emplace( curViewer.Connection, FVector( ForceInitToZero ) );
                gatherInfoForConnection = &lastLocationArray[ idx ];
            }
        }

        FVector lastLocationForConnection = gatherInfoForConnection ? gatherInfoForConnection->LastLocation : clampedViewLoc;
        //@todo: if this is clamp view loc this is now redundant...
        lastLocationForConnection = lastLocationForConnection.BoundToCube( HALF_WORLD_MAX );

        FPlayerHexCellInfo newPlayerCell;
        newPlayerCell.curHex = FHexModel::HexRound( FHexModel::PixelToHex( sTmpHexLayout, FPoint( clampedViewLoc ) ) );
        newPlayerCell.prevHex = FHexModel::HexRound( FHexModel::PixelToHex( sTmpHexLayout, FPoint( lastLocationForConnection ) ) );

        // Add this to things we consider later.
        activeHexCells.Add( newPlayerCell );

        // If we have not operated on this cell yet (meaning it's not shared by anyone else), gather for it.
        if ( !uniqueCurrentHexes.Contains( newPlayerCell.curHex ) )
        {
            UReplicationGraphNode_HexCell* cellNode = mHexCells.FindRef( newPlayerCell.curHex );
            if ( cellNode )
                cellNode->GatherActorListsForConnection( Params );

            uniqueCurrentHexes.Add( newPlayerCell.curHex );
        }
    }

    if ( bDestroyDormantDynamicActors && CVar_HexRepGraph_DormantDynamicActorsDestruction > 0 )
    {
        FActorRepListRefView prevDormantActorList;

        // Process and create the dormancy list for the active grid for this user
        for ( const FPlayerHexCellInfo& hexCellInfo : activeHexCells )
        {
            // The idea is that if the previous location is a current location for any other user, we do not bother to do operations on this cell
            // However, if the current location matches with a current location of another user, continue anyways.
            // As above, if the grid cell changed this gather and is not in current use by any other viewer

            // TODO: There is a potential list gathering redundancy if two actors share the same current and previous cell information
            // but this should just result in a wasted cycle if anything.
            if ( (hexCellInfo.curHex != hexCellInfo.prevHex) && !uniqueCurrentHexes.Contains( hexCellInfo.prevHex ) )
            {
                RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexSpatialization2D_CellChangeDormantRelevancy );

                FActorRepListRefView dormantActorList;

                if ( UReplicationGraphNode_HexCell* hexCell = *mHexCells.Find( hexCellInfo.curHex ) )
                {
                    if ( UReplicationGraphNode_DormancyNode* dormancyNode = hexCell->GetDormancyNode( ) )
                        dormancyNode->ConditionalGatherDormantDynamicActors( dormantActorList, Params, nullptr );
                }

                // Determine dormant actors for our last location. Do not add actors if they are relevant to anyone.
                if ( UReplicationGraphNode_HexCell* prevCell = *mHexCells.Find( hexCellInfo.prevHex ) )
                {
                    if ( UReplicationGraphNode_DormancyNode* dormancyNode = prevCell->GetDormancyNode( ) )
                        dormancyNode->ConditionalGatherDormantDynamicActors( prevDormantActorList, Params, &dormantActorList, true );
                }
            }
        }

        // Now process the previous dormant list to handle destruction
        if ( prevDormantActorList.IsValid( ) )
        {
            // any previous dormant actors not in the current node dormant list
            for ( FActorRepListType& actor : prevDormantActorList )
            {
                Params.ConnectionManager.NotifyAddDormantDestructionInfo( actor );

                if ( FConnectionReplicationActorInfo* actorInfo = Params.ConnectionManager.ActorInfoMap.Find( actor ) )
                {
                    actorInfo->bDormantOnConnection = false;
                    // Ideally, no actor info outside this list should be set to true, so we don't have to worry about resetting them.
                    // However we could consider iterating through the actor map to reset all of them.
                    actorInfo->bGridSpatilization_AlreadyDormant = false;

                    // add back to connection specific dormancy nodes
                    TArray<UReplicationGraphNode_HexCell*> hexCells = GetHexCellCoverage( actor->GetActorLocation( ), actorInfo->GetCullDistance( ) );

                    for ( auto hexCell : hexCells )
                    {
                        if ( UReplicationGraphNode_DormancyNode* dormancyNode = hexCell->GetDormancyNode( ) )
                        {
                            // Only notify the connection node if this client was previously inside the cell.
                            if ( UReplicationGraphNode_ConnectionDormancyNode* connectionDormancyNode = dormancyNode->GetExistingConnectionNode( Params ) )
                                connectionDormancyNode->NotifyActorDormancyFlush( actor );
                        }
                    }
                }
            }
        }
    }
}

void UReplicationGraphNode_HexSpatialization2D::NotifyActorCullDistChange( AActor* actor, FGlobalActorReplicationInfo& actorGlobalInfo, float oldDist )
{
    UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexSpatialization2D_NotifyActorCullDistChange );

    // If this actor is statically spatialized then we need to remove it and readd it (this is a little wasteful but in practice not common/only happens at startup)
    if ( FStaticActorInfo* staticActorInfo = mStaticSpatializedActors.Find( actor ) )
    {
        // Remove with old distance
        auto hexCells = GetHexCellCoverage( actorGlobalInfo.WorldLocation, oldDist );
        for ( auto hexCell : hexCells )
            hexCell->RemoveStaticActor( staticActorInfo->actorInfo, actorGlobalInfo, actorGlobalInfo.bWantsToBeDormant );

        // Add new distances (there is some waste here but this hopefully doesn't happen much at runtime!)
        hexCells = GetHexCellCoverage( actorGlobalInfo.WorldLocation, actorGlobalInfo.Settings.GetCullDistance( ) );
        for ( auto hexCell : hexCells )
            hexCell->AddStaticActor( staticActorInfo->actorInfo, actorGlobalInfo, staticActorInfo->bDormancyDriven );
    }
    else if ( FDynamicActorInfo* dynamicActorInfo = mDynamicSpatializedActors.Find( actor ) )
    {
        // Pull dynamic actor out of the grid. We will put him back on the next gather
        auto hexCells = GetHexCellCoverage( actorGlobalInfo.WorldLocation, oldDist );
        for ( auto hexCell : hexCells )
            hexCell->RemoveDynamicActor( dynamicActorInfo->actorInfo );
    }
    else
    {

#if !UE_BUILD_SHIPPING
        // Might be in the pending init list
        if ( mPendingStaticSpatializedActors.FindByKey( actor ) == nullptr )
        {
            UE_LOG( LogHexRepGraph, Warning, TEXT( "UReplicationGraphNode_HexSpatialization2D::NotifyActorCullDistChange. %s Changed Cull Distance (%.2f -> %.2f) but is not in static or dynamic actor lists. %s" ), *actor->GetPathName( ), oldDist, actorGlobalInfo.Settings.GetCullDistance( ), *GetPathName( ) );

            // Search the entire grid. This is slow so only enabled if verify is on.
            if ( CVar_RepGraph_Verify )
                RepGraphVerifySlow( actor, actorGlobalInfo, true );
        }
#endif
    }
}

UReplicationGraphNode_HexCell* UReplicationGraphNode_HexSpatialization2D::CreateHexCellNode( UReplicationGraphNode_HexCell*& hexCellPtr, const FHex& hex )
{
    UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexSpatialization2D_CreateHexCellNode );

    if ( hexCellPtr != nullptr )
        return hexCellPtr;

    if ( mCreateCellNodeOverrideFun )
        hexCellPtr = mCreateCellNodeOverrideFun( this );
    else
        hexCellPtr = CreateChildNode<UReplicationGraphNode_HexCell>( );

    hexCellPtr->SetHex( hex );

    return hexCellPtr;
}

TArray<UReplicationGraphNode_HexCell*> UReplicationGraphNode_HexSpatialization2D::GetHexCellCoverage( const FVector& location, const float cullDistance )
{
    //UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    //RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexSpatialization2D_GetHexCellCoverage );

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    if ( cullDistance <= 0.f )
    {
        UE_LOG( LogHexRepGraph, Warning, TEXT( "GetHexCellCoverage called on %s when its cullDistance = %.2f. (Must be > 0)" ), *location.ToCompactString( ), cullDistance );
    }
#endif

    FVector clampedLocation = location;

    // Sanity check the actor's location. If it's garbage, we could end up with a gigantic allocation in GetHexCells as we adjust the grid.
    if ( location.X < -HALF_WORLD_MAX || location.X > HALF_WORLD_MAX ||
         location.Y < -HALF_WORLD_MAX || location.Y > HALF_WORLD_MAX ||
         location.Z < -HALF_WORLD_MAX || location.Z > HALF_WORLD_MAX )
    {
        UE_LOG( LogHexRepGraph, Warning, TEXT( "GetHexCellCoverage: location %s is outside world bounds. Clamping grid location to world bounds." ), *location.ToString( ) );
        clampedLocation = location.BoundToCube( HALF_WORLD_MAX );
    }

    TArray<UReplicationGraphNode_HexCell*> result;

    const TArray<FHex> hexCoverage = FHexModel::HexCoverage( sTmpHexLayout, FVector2D( clampedLocation ), cullDistance, sTmpMaxHexDistToCenter, sTmpMaxRing );

    for ( const FHex& hex : hexCoverage )
    {
        UReplicationGraphNode_HexCell*& hexCell = mHexCells.FindOrAdd( hex );
        if ( !hexCell )
            CreateHexCellNode( hexCell, hex );

        result.AddUnique( hexCell );
    }

    return result;
}

void UReplicationGraphNode_HexSpatialization2D::RepGraphVerifySlow( const FActorRepListType& actor, FGlobalActorReplicationInfo& actorRepInfo, bool mustExists )
{
    UE_LOG( LogHexRepGraph, Display, TEXT( __FUNCTION__ ) );
    RG_QUICK_SCOPE_CYCLE_COUNTER( UReplicationGraphNode_HexSpatialization2D_RepGraphVerifySlow );

    // Verify this actor is in no nodes. This is pretty slow!
    bool exists = false;
    for ( auto& hexCell : mHexCells )
    {
        TArray<AActor*> allActors;
        hexCell.Value->GetAllActorsInNode_Debugging( allActors );

        exists = allActors.Contains( actor );

        if ( mustExists && exists )
            break;

        if ( !mustExists )
            ensureMsgf( exists == false, TEXT( "actor still in a node after removal!. %s. Removal Location: %s" ), *hexCell.Value->GetPathName( ),
                        *actorRepInfo.WorldLocation.ToString( ) );
    }

    if ( mustExists )
        ensureMsgf( exists == true, TEXT( "actor is not in any node after adding!. Adding Location: %s" ), *actorRepInfo.WorldLocation.ToString( ) );
}

// -------------------------------------------------------