// Fill out your copyright notice in the Description page of Project Settings.


#include "HexReplicationGraph.h"
#if 0
#include "Net/UnrealNetwork.h"
#include "Engine/LevelStreaming.h"
#include "EngineUtils.h"
#include "CoreGlobals.h"

#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameState.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LevelScriptActor.h"
#include "HexSystemCharacter.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerCategoryReplicator.h"
#endif

//#include "Online/ShooterPlayerState.h"
//#include "Weapons/ShooterWeapon.h"
//#include "Pickups/ShooterPickup.h"

DEFINE_LOG_CATEGORY( LogHexReplicationGraph );

float CVar_ShooterRepGraph_DestructionInfoMaxDist = 30000.f;
static FAutoConsoleVariableRef CVarShooterRepGraphDestructMaxDist( TEXT( "ShooterRepGraph.DestructInfo.MaxDist" ), CVar_ShooterRepGraph_DestructionInfoMaxDist, TEXT( "Max distance (not squared) to rep destruct infos at" ), ECVF_Default );

int32 CVar_ShooterRepGraph_DisplayClientLevelStreaming = 0;
static FAutoConsoleVariableRef CVarShooterRepGraphDisplayClientLevelStreaming( TEXT( "ShooterRepGraph.DisplayClientLevelStreaming" ), CVar_ShooterRepGraph_DisplayClientLevelStreaming, TEXT( "" ), ECVF_Default );

float CVar_ShooterRepGraph_CellSize = 10000.f;
static FAutoConsoleVariableRef CVarShooterRepGraphCellSize( TEXT( "ShooterRepGraph.CellSize" ), CVar_ShooterRepGraph_CellSize, TEXT( "" ), ECVF_Default );

// Essentially "Min X" for replication. This is just an initial value. The system will reset itself if actors appears outside of this.
float CVar_ShooterRepGraph_SpatialBiasX = -150000.f;
static FAutoConsoleVariableRef CVarShooterRepGraphSpatialBiasX( TEXT( "ShooterRepGraph.SpatialBiasX" ), CVar_ShooterRepGraph_SpatialBiasX, TEXT( "" ), ECVF_Default );

// Essentially "Min Y" for replication. This is just an initial value. The system will reset itself if actors appears outside of this.
float CVar_ShooterRepGraph_SpatialBiasY = -200000.f;
static FAutoConsoleVariableRef CVarShooterRepSpatialBiasY( TEXT( "ShooterRepGraph.SpatialBiasY" ), CVar_ShooterRepGraph_SpatialBiasY, TEXT( "" ), ECVF_Default );

// How many buckets to spread dynamic, spatialized actors across. High number = more buckets = smaller effective replication frequency. This happens before individual actors do their own NetUpdateFrequency check.
int32 CVar_ShooterRepGraph_DynamicActorFrequencyBuckets = 3;
static FAutoConsoleVariableRef CVarShooterRepDynamicActorFrequencyBuckets( TEXT( "ShooterRepGraph.DynamicActorFrequencyBuckets" ), CVar_ShooterRepGraph_DynamicActorFrequencyBuckets, TEXT( "" ), ECVF_Default );

int32 CVar_ShooterRepGraph_DisableSpatialRebuilds = 1;
static FAutoConsoleVariableRef CVarShooterRepDisableSpatialRebuilds( TEXT( "ShooterRepGraph.DisableSpatialRebuilds" ), CVar_ShooterRepGraph_DisableSpatialRebuilds, TEXT( "" ), ECVF_Default );

// ----------------------------------------------------------------------------------------------------------


UHexReplicationGraph::UHexReplicationGraph( )
{
}

void InitClassReplicationInfo( FClassReplicationInfo& Info, UClass* Class, bool bSpatialize, float ServerMaxTickRate )
{
    AActor* CDO = Class->GetDefaultObject<AActor>( );
    if ( bSpatialize )
    {
        Info.SetCullDistanceSquared( CDO->NetCullDistanceSquared );
        UE_LOG( LogHexReplicationGraph, Log, TEXT( "Setting cull distance for %s to %f (%f)" ), *Class->GetName( ), Info.GetCullDistanceSquared( ), Info.GetCullDistance( ) );
    }

    Info.ReplicationPeriodFrame = FMath::Max<uint32>( ( uint32 )FMath::RoundToFloat( ServerMaxTickRate / CDO->NetUpdateFrequency ), 1 );

    UClass* NativeClass = Class;
    while ( !NativeClass->IsNative( ) && NativeClass->GetSuperClass( ) && NativeClass->GetSuperClass( ) != AActor::StaticClass( ) )
    {
        NativeClass = NativeClass->GetSuperClass( );
    }

    UE_LOG( LogHexReplicationGraph, Log, TEXT( "Setting replication period for %s (%s) to %d frames (%.2f)" ), *Class->GetName( ), *NativeClass->GetName( ), Info.ReplicationPeriodFrame, CDO->NetUpdateFrequency );
}

void UHexReplicationGraph::ResetGameWorldState( )
{
    Super::ResetGameWorldState( );

    AlwaysRelevantStreamingLevelActors.Empty( );

    for ( UNetReplicationGraphConnection* ConnManager : Connections )
    {
        for ( UReplicationGraphNode* ConnectionNode : ConnManager->GetConnectionGraphNodes( ) )
        {
            if ( UHexReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevantConnectionNode = Cast<UHexReplicationGraphNode_AlwaysRelevant_ForConnection>( ConnectionNode ) )
            {
                AlwaysRelevantConnectionNode->ResetGameWorldState( );
            }
        }
    }

    for ( UNetReplicationGraphConnection* ConnManager : PendingConnections )
    {
        for ( UReplicationGraphNode* ConnectionNode : ConnManager->GetConnectionGraphNodes( ) )
        {
            if ( UHexReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevantConnectionNode = Cast<UHexReplicationGraphNode_AlwaysRelevant_ForConnection>( ConnectionNode ) )
            {
                AlwaysRelevantConnectionNode->ResetGameWorldState( );
            }
        }
    }
}

void UHexReplicationGraph::InitGlobalActorClassSettings( )
{
    Super::InitGlobalActorClassSettings( );

    // ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    // Programatically build the rules.
    // ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    auto AddInfo = [ & ]( UClass* Class, EClassRepNodeMapping Mapping )
    {
        ClassRepNodePolicies.Set( Class, Mapping );
    };

    //AddInfo( AShooterWeapon::StaticClass( ), EClassRepNodeMapping::NotRouted );				// Handled via DependantActor replication (Pawn)
    AddInfo( ALevelScriptActor::StaticClass( ), EClassRepNodeMapping::NotRouted );				// Not needed
    AddInfo( APlayerState::StaticClass( ), EClassRepNodeMapping::NotRouted );				// Special cased via UHexReplicationGraphNode_PlayerStateFrequencyLimiter
    AddInfo( AReplicationGraphDebugActor::StaticClass( ), EClassRepNodeMapping::NotRouted );				// Not needed. Replicated special case inside RepGraph
    AddInfo( AInfo::StaticClass( ), EClassRepNodeMapping::RelevantAllConnections );	// Non spatialized, relevant to all
    //AddInfo( AShooterPickup::StaticClass( ), EClassRepNodeMapping::Spatialize_Static );		// Spatialized and never moves. Routes to GridNode.

#if WITH_GAMEPLAY_DEBUGGER
    AddInfo( AGameplayDebuggerCategoryReplicator::StaticClass( ), EClassRepNodeMapping::NotRouted );				// Replicated via UHexReplicationGraphNode_AlwaysRelevant_ForConnection
#endif

    TArray<UClass*> AllReplicatedClasses;

    for ( TObjectIterator<UClass> It; It; ++It )
    {
        UClass* Class = *It;
        AActor* ActorCDO = Cast<AActor>( Class->GetDefaultObject( ) );
        if ( !ActorCDO || !ActorCDO->GetIsReplicated( ) )
        {
            continue;
        }

        // Skip SKEL and REINST classes.
        if ( Class->GetName( ).StartsWith( TEXT( "SKEL_" ) ) || Class->GetName( ).StartsWith( TEXT( "REINST_" ) ) )
        {
            continue;
        }

        // --------------------------------------------------------------------
        // This is a replicated class. Save this off for the second pass below
        // --------------------------------------------------------------------

        AllReplicatedClasses.Add( Class );

        // Skip if already in the map (added explicitly)
        if ( ClassRepNodePolicies.Contains( Class, false ) )
        {
            continue;
        }

        auto ShouldSpatialize = [ ]( const AActor* CDO )
        {
            return CDO->GetIsReplicated( ) && ( !( CDO->bAlwaysRelevant || CDO->bOnlyRelevantToOwner || CDO->bNetUseOwnerRelevancy ) );
        };

        auto GetLegacyDebugStr = [ ]( const AActor* CDO )
        {
            return FString::Printf( TEXT( "%s [%d/%d/%d]" ), *CDO->GetClass( )->GetName( ), CDO->bAlwaysRelevant, CDO->bOnlyRelevantToOwner, CDO->bNetUseOwnerRelevancy );
        };

        // Only handle this class if it differs from its super. There is no need to put every child class explicitly in the graph class mapping
        UClass* SuperClass = Class->GetSuperClass( );
        if ( AActor* SuperCDO = Cast<AActor>( SuperClass->GetDefaultObject( ) ) )
        {
            if ( SuperCDO->GetIsReplicated( ) == ActorCDO->GetIsReplicated( )
                 && SuperCDO->bAlwaysRelevant == ActorCDO->bAlwaysRelevant
                 && SuperCDO->bOnlyRelevantToOwner == ActorCDO->bOnlyRelevantToOwner
                 && SuperCDO->bNetUseOwnerRelevancy == ActorCDO->bNetUseOwnerRelevancy
                 )
            {
                continue;
            }

            if ( ShouldSpatialize( ActorCDO ) == false && ShouldSpatialize( SuperCDO ) == true )
            {
                UE_LOG( LogHexReplicationGraph, Log, TEXT( "Adding %s to NonSpatializedChildClasses. (Parent: %s)" ), *GetLegacyDebugStr( ActorCDO ), *GetLegacyDebugStr( SuperCDO ) );
                NonSpatializedChildClasses.Add( Class );
            }
        }

        if ( ShouldSpatialize( ActorCDO ) )
        {
            AddInfo( Class, EClassRepNodeMapping::Spatialize_Dynamic );
        }
        else if ( ActorCDO->bAlwaysRelevant && !ActorCDO->bOnlyRelevantToOwner )
        {
            AddInfo( Class, EClassRepNodeMapping::RelevantAllConnections );
        }
    }

    // -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
    // Setup FClassReplicationInfo. This is essentially the per class replication settings. Some we set explicitly, the rest we are setting via looking at the legacy settings on AActor.
    // -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

    TArray<UClass*> ExplicitlySetClasses;
    auto SetClassInfo = [ & ]( UClass* Class, const FClassReplicationInfo& Info )
    {
        GlobalActorReplicationInfoMap.SetClassInfo( Class, Info ); ExplicitlySetClasses.Add( Class );
    };

    FClassReplicationInfo PawnClassRepInfo;
    PawnClassRepInfo.DistancePriorityScale = 1.f;
    PawnClassRepInfo.StarvationPriorityScale = 1.f;
    PawnClassRepInfo.ActorChannelFrameTimeout = 4;
    PawnClassRepInfo.SetCullDistanceSquared( 15000.f * 15000.f ); // Yuck
    SetClassInfo( APawn::StaticClass( ), PawnClassRepInfo );

    FClassReplicationInfo PlayerStateRepInfo;
    PlayerStateRepInfo.DistancePriorityScale = 0.f;
    PlayerStateRepInfo.ActorChannelFrameTimeout = 0;
    SetClassInfo( APlayerState::StaticClass( ), PlayerStateRepInfo );

    UReplicationGraphNode_ActorListFrequencyBuckets::DefaultSettings.ListSize = 12;

    // Set FClassReplicationInfo based on legacy settings from all replicated classes
    for ( UClass* ReplicatedClass : AllReplicatedClasses )
    {
        if ( ExplicitlySetClasses.FindByPredicate( [ & ]( const UClass* SetClass )
        {
            return ReplicatedClass->IsChildOf( SetClass );
        } ) != nullptr )
        {
            continue;
        }

        const bool bClassIsSpatialized = IsSpatialized( ClassRepNodePolicies.GetChecked( ReplicatedClass ) );

        FClassReplicationInfo ClassInfo;
        InitClassReplicationInfo( ClassInfo, ReplicatedClass, bClassIsSpatialized, NetDriver->NetServerMaxTickRate );
        GlobalActorReplicationInfoMap.SetClassInfo( ReplicatedClass, ClassInfo );
    }


    // Print out what we came up with
    UE_LOG( LogHexReplicationGraph, Log, TEXT( "" ) );
    UE_LOG( LogHexReplicationGraph, Log, TEXT( "Class Routing Map: " ) );
    UEnum* Enum = StaticEnum<EClassRepNodeMapping>( );
    for ( auto ClassMapIt = ClassRepNodePolicies.CreateIterator( ); ClassMapIt; ++ClassMapIt )
    {
        UClass* Class = CastChecked<UClass>( ClassMapIt.Key( ).ResolveObjectPtr( ) );
        const EClassRepNodeMapping Mapping = ClassMapIt.Value( );

        // Only print if different than native class
        UClass* ParentNativeClass = GetParentNativeClass( Class );
        const EClassRepNodeMapping* ParentMapping = ClassRepNodePolicies.Get( ParentNativeClass );
        if ( ParentMapping && Class != ParentNativeClass && Mapping == *ParentMapping )
        {
            continue;
        }

        UE_LOG( LogHexReplicationGraph, Log, TEXT( "  %s (%s) -> %s" ), *Class->GetName( ), *GetNameSafe( ParentNativeClass ), *Enum->GetNameStringByValue( static_cast< uint32 >( Mapping ) ) );
    }

    UE_LOG( LogHexReplicationGraph, Log, TEXT( "" ) );
    UE_LOG( LogHexReplicationGraph, Log, TEXT( "Class Settings Map: " ) );
    FClassReplicationInfo DefaultValues;
    for ( auto ClassRepInfoIt = GlobalActorReplicationInfoMap.CreateClassMapIterator( ); ClassRepInfoIt; ++ClassRepInfoIt )
    {
        UClass* Class = CastChecked<UClass>( ClassRepInfoIt.Key( ).ResolveObjectPtr( ) );
        const FClassReplicationInfo& ClassInfo = ClassRepInfoIt.Value( );
        UE_LOG( LogHexReplicationGraph, Log, TEXT( "  %s (%s) -> %s" ), *Class->GetName( ), *GetNameSafe( GetParentNativeClass( Class ) ), *ClassInfo.BuildDebugStringDelta( ) );
    }


    // Rep destruct infos based on CVar value
    DestructInfoMaxDistanceSquared = CVar_ShooterRepGraph_DestructionInfoMaxDist * CVar_ShooterRepGraph_DestructionInfoMaxDist;

    // -------------------------------------------------------
    //	Register for game code callbacks.
    //	This could have been done the other way: E.g, AMyGameActor could do GetNetDriver()->GetReplicationDriver<UHexReplicationGraph>()->OnMyGameEvent etc.
    //	This way at least keeps the rep graph out of game code directly and allows rep graph to exist in its own module
    //	So for now, erring on the side of a cleaning dependencies between classes.
    // -------------------------------------------------------

    //AHexSystemCharacter::NotifyEquipWeapon.AddUObject( this, &UHexReplicationGraph::OnCharacterEquipWeapon );
    //AHexSystemCharacter::NotifyUnEquipWeapon.AddUObject( this, &UHexReplicationGraph::OnCharacterUnEquipWeapon );

#if WITH_GAMEPLAY_DEBUGGER
    AGameplayDebuggerCategoryReplicator::NotifyDebuggerOwnerChange.AddUObject( this, &UHexReplicationGraph::OnGameplayDebuggerOwnerChange );
#endif
}

void UHexReplicationGraph::InitGlobalGraphNodes( )
{
    // Preallocate some replication lists.
    PreAllocateRepList( 3, 12 );
    PreAllocateRepList( 6, 12 );
    PreAllocateRepList( 128, 64 );
    PreAllocateRepList( 512, 16 );

    // -----------------------------------------------
    //	Spatial Actors
    // -----------------------------------------------

    GridNode = CreateNewNode<UReplicationGraphNode_GridSpatialization2D>( );
    GridNode->CellSize = CVar_ShooterRepGraph_CellSize;
    GridNode->SpatialBias = FVector2D( CVar_ShooterRepGraph_SpatialBiasX, CVar_ShooterRepGraph_SpatialBiasY );

    if ( CVar_ShooterRepGraph_DisableSpatialRebuilds )
    {
        GridNode->AddSpatialRebuildBlacklistClass( AActor::StaticClass( ) ); // Disable All spatial rebuilding
    }

    AddGlobalGraphNode( GridNode );

    // -----------------------------------------------
    //	Always Relevant (to everyone) Actors
    // -----------------------------------------------
    AlwaysRelevantNode = CreateNewNode<UReplicationGraphNode_ActorList>( );
    AddGlobalGraphNode( AlwaysRelevantNode );

    // -----------------------------------------------
    //	Player State specialization. This will return a rolling subset of the player states to replicate
    // -----------------------------------------------
    UHexReplicationGraphNode_PlayerStateFrequencyLimiter* PlayerStateNode = CreateNewNode<UHexReplicationGraphNode_PlayerStateFrequencyLimiter>( );
    AddGlobalGraphNode( PlayerStateNode );
}

void UHexReplicationGraph::InitConnectionGraphNodes( UNetReplicationGraphConnection* RepGraphConnection )
{
    Super::InitConnectionGraphNodes( RepGraphConnection );

    UHexReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevantConnectionNode = CreateNewNode<UHexReplicationGraphNode_AlwaysRelevant_ForConnection>( );

    // This node needs to know when client levels go in and out of visibility
    RepGraphConnection->OnClientVisibleLevelNameAdd.AddUObject( AlwaysRelevantConnectionNode, &UHexReplicationGraphNode_AlwaysRelevant_ForConnection::OnClientLevelVisibilityAdd );
    RepGraphConnection->OnClientVisibleLevelNameRemove.AddUObject( AlwaysRelevantConnectionNode, &UHexReplicationGraphNode_AlwaysRelevant_ForConnection::OnClientLevelVisibilityRemove );

    AddConnectionGraphNode( AlwaysRelevantConnectionNode, RepGraphConnection );
}

EClassRepNodeMapping UHexReplicationGraph::GetMappingPolicy( UClass* Class )
{
    EClassRepNodeMapping* PolicyPtr = ClassRepNodePolicies.Get( Class );
    EClassRepNodeMapping Policy = PolicyPtr ? *PolicyPtr : EClassRepNodeMapping::NotRouted;
    return Policy;
}

void UHexReplicationGraph::RouteAddNetworkActorToNodes( const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& GlobalInfo )
{
    EClassRepNodeMapping Policy = GetMappingPolicy( ActorInfo.Class );
    switch ( Policy )
    {
    case EClassRepNodeMapping::NotRouted:
    {
        break;
    }

    case EClassRepNodeMapping::RelevantAllConnections:
    {
        if ( ActorInfo.StreamingLevelName == NAME_None )
        {
            AlwaysRelevantNode->NotifyAddNetworkActor( ActorInfo );
        }
        else
        {
            FActorRepListRefView& RepList = AlwaysRelevantStreamingLevelActors.FindOrAdd( ActorInfo.StreamingLevelName );
            RepList.PrepareForWrite( );
            RepList.ConditionalAdd( ActorInfo.Actor );
        }
        break;
    }

    case EClassRepNodeMapping::Spatialize_Static:
    {
        GridNode->AddActor_Static( ActorInfo, GlobalInfo );
        break;
    }

    case EClassRepNodeMapping::Spatialize_Dynamic:
    {
        GridNode->AddActor_Dynamic( ActorInfo, GlobalInfo );
        break;
    }

    case EClassRepNodeMapping::Spatialize_Dormancy:
    {
        GridNode->AddActor_Dormancy( ActorInfo, GlobalInfo );
        break;
    }
    };
}

void UHexReplicationGraph::RouteRemoveNetworkActorToNodes( const FNewReplicatedActorInfo& ActorInfo )
{
    EClassRepNodeMapping Policy = GetMappingPolicy( ActorInfo.Class );
    switch ( Policy )
    {
    case EClassRepNodeMapping::NotRouted:
    {
        break;
    }

    case EClassRepNodeMapping::RelevantAllConnections:
    {
        if ( ActorInfo.StreamingLevelName == NAME_None )
        {
            AlwaysRelevantNode->NotifyRemoveNetworkActor( ActorInfo );
        }
        else
        {
            FActorRepListRefView& RepList = AlwaysRelevantStreamingLevelActors.FindChecked( ActorInfo.StreamingLevelName );
            if ( RepList.Remove( ActorInfo.Actor ) == false )
            {
                UE_LOG( LogHexReplicationGraph, Warning, TEXT( "Actor %s was not found in AlwaysRelevantStreamingLevelActors list. LevelName: %s" ), *GetActorRepListTypeDebugString( ActorInfo.Actor ), *ActorInfo.StreamingLevelName.ToString( ) );
            }
        }
        break;
    }

    case EClassRepNodeMapping::Spatialize_Static:
    {
        GridNode->RemoveActor_Static( ActorInfo );
        break;
    }

    case EClassRepNodeMapping::Spatialize_Dynamic:
    {
        GridNode->RemoveActor_Dynamic( ActorInfo );
        break;
    }

    case EClassRepNodeMapping::Spatialize_Dormancy:
    {
        GridNode->RemoveActor_Dormancy( ActorInfo );
        break;
    }
    };
}

// Since we listen to global (static) events, we need to watch out for cross world broadcasts (PIE)
#if WITH_EDITOR
#define CHECK_WORLDS(X) if(X->GetWorld() != GetWorld()) return;
#else
#define CHECK_WORLDS(X)
#endif

//void UHexReplicationGraph::OnCharacterEquipWeapon( AHexSystemCharacter* Character, AShooterWeapon* NewWeapon )
//{
//    if ( Character && NewWeapon )
//    {
//        CHECK_WORLDS( Character );
//
//        GlobalActorReplicationInfoMap.AddDependentActor( Character, NewWeapon );
//    }
//}
//
//void UHexReplicationGraph::OnCharacterUnEquipWeapon( AHexSystemCharacter* Character, AShooterWeapon* OldWeapon )
//{
//    if ( Character && OldWeapon )
//    {
//        CHECK_WORLDS( Character );
//
//        GlobalActorReplicationInfoMap.RemoveDependentActor( Character, OldWeapon );
//    }
//}

#if WITH_GAMEPLAY_DEBUGGER
void UHexReplicationGraph::OnGameplayDebuggerOwnerChange( AGameplayDebuggerCategoryReplicator* Debugger, APlayerController* OldOwner )
{
    auto GetAlwaysRelevantForConnectionNode = [ & ]( APlayerController* Controller ) -> UHexReplicationGraphNode_AlwaysRelevant_ForConnection*
    {
        if ( OldOwner )
        {
            if ( UNetConnection* NetConnection = OldOwner->GetNetConnection( ) )
            {
                if ( UNetReplicationGraphConnection* GraphConnection = FindOrAddConnectionManager( NetConnection ) )
                {
                    for ( UReplicationGraphNode* ConnectionNode : GraphConnection->GetConnectionGraphNodes( ) )
                    {
                        if ( UHexReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevantConnectionNode = Cast<UHexReplicationGraphNode_AlwaysRelevant_ForConnection>( ConnectionNode ) )
                        {
                            return AlwaysRelevantConnectionNode;
                        }
                    }

                }
            }
        }

        return nullptr;
    };

    if ( UHexReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevantConnectionNode = GetAlwaysRelevantForConnectionNode( OldOwner ) )
    {
        AlwaysRelevantConnectionNode->GameplayDebugger = nullptr;
    }

    if ( UHexReplicationGraphNode_AlwaysRelevant_ForConnection* AlwaysRelevantConnectionNode = GetAlwaysRelevantForConnectionNode( Debugger->GetReplicationOwner( ) ) )
    {
        AlwaysRelevantConnectionNode->GameplayDebugger = Debugger;
    }
}
#endif

// ------------------------------------------------------------------------------

void UHexReplicationGraphNode_AlwaysRelevant_ForConnection::ResetGameWorldState( )
{
    AlwaysRelevantStreamingLevelsNeedingReplication.Empty( );
}

void UHexReplicationGraphNode_AlwaysRelevant_ForConnection::GatherActorListsForConnection( const FConnectionGatherActorListParameters& Params )
{
    QUICK_SCOPE_CYCLE_COUNTER( UHexReplicationGraphNode_AlwaysRelevant_ForConnection_GatherActorListsForConnection );

    UHexReplicationGraph* ShooterGraph = CastChecked<UHexReplicationGraph>( GetOuter( ) );

    ReplicationActorList.Reset( );

    auto ResetActorCullDistance = [ & ]( AActor* ActorToSet, AActor*& LastActor )
    {

        if ( ActorToSet != LastActor )
        {
            LastActor = ActorToSet;

            UE_LOG( LogHexReplicationGraph, Verbose, TEXT( "Setting pawn cull distance to 0. %s" ), *ActorToSet->GetName( ) );
            FConnectionReplicationActorInfo& ConnectionActorInfo = Params.ConnectionManager.ActorInfoMap.FindOrAdd( ActorToSet );
            ConnectionActorInfo.SetCullDistanceSquared( 0.f );
        }
    };

    for ( const FNetViewer& CurViewer : Params.Viewers )
    {
        ReplicationActorList.ConditionalAdd( CurViewer.InViewer );
        ReplicationActorList.ConditionalAdd( CurViewer.ViewTarget );

        if ( APlayerController* PC = Cast<APlayerController>( CurViewer.InViewer ) )
        {
            // 50% throttling of PlayerStates.
            const bool bReplicatePS = ( Params.ConnectionManager.ConnectionOrderNum % 2 ) == ( Params.ReplicationFrameNum % 2 );
            if ( bReplicatePS )
            {
                // Always return the player state to the owning player. Simulated proxy player states are handled by UHexReplicationGraphNode_PlayerStateFrequencyLimiter
                if ( APlayerState* PS = PC->PlayerState )
                {
                    if ( !bInitializedPlayerState )
                    {
                        bInitializedPlayerState = true;
                        FConnectionReplicationActorInfo& ConnectionActorInfo = Params.ConnectionManager.ActorInfoMap.FindOrAdd( PS );
                        ConnectionActorInfo.ReplicationPeriodFrame = 1;
                    }

                    ReplicationActorList.ConditionalAdd( PS );
                }
            }

            FAlwaysRelevantActorInfo* LastData = PastRelevantActors.FindByKey<UNetConnection*>( CurViewer.Connection );

            // We've not seen this actor before, go ahead and add them.
            if ( LastData == nullptr )
            {
                FAlwaysRelevantActorInfo NewActorInfo;
                NewActorInfo.Connection = CurViewer.Connection;
                LastData = &( PastRelevantActors[ PastRelevantActors.Add( NewActorInfo ) ] );
            }

            check( LastData != nullptr );

            if ( AHexSystemCharacter* Pawn = Cast<AHexSystemCharacter>( PC->GetPawn( ) ) )
            {
                ResetActorCullDistance( Pawn, LastData->LastViewer );

                if ( Pawn != CurViewer.ViewTarget )
                {
                    ReplicationActorList.ConditionalAdd( Pawn );
                }

                /*int32 InventoryCount = Pawn->GetInventoryCount( );
                for ( int32 i = 0; i < InventoryCount; ++i )
                {
                    AShooterWeapon* Weapon = Pawn->GetInventoryWeapon( i );
                    if ( Weapon )
                    {
                        ReplicationActorList.ConditionalAdd( Weapon );
                    }
                }*/
            }

            if ( AHexSystemCharacter* ViewTargetPawn = Cast<AHexSystemCharacter>( CurViewer.ViewTarget ) )
            {
                ResetActorCullDistance( ViewTargetPawn, LastData->LastViewTarget );
            }
        }
    }

    PastRelevantActors.RemoveAll( [ & ]( FAlwaysRelevantActorInfo& RelActorInfo )
    {
        return RelActorInfo.Connection == nullptr;
    } );

    Params.OutGatheredReplicationLists.AddReplicationActorList( ReplicationActorList );

    // Always relevant streaming level actors.
    FPerConnectionActorInfoMap& ConnectionActorInfoMap = Params.ConnectionManager.ActorInfoMap;

    TMap<FName, FActorRepListRefView>& AlwaysRelevantStreamingLevelActors = ShooterGraph->AlwaysRelevantStreamingLevelActors;

    for ( int32 Idx = AlwaysRelevantStreamingLevelsNeedingReplication.Num( ) - 1; Idx >= 0; --Idx )
    {
        const FName& StreamingLevel = AlwaysRelevantStreamingLevelsNeedingReplication[ Idx ];

        FActorRepListRefView* Ptr = AlwaysRelevantStreamingLevelActors.Find( StreamingLevel );
        if ( Ptr == nullptr )
        {
            // No always relevant lists for that level
            UE_CLOG( CVar_ShooterRepGraph_DisplayClientLevelStreaming > 0, LogHexReplicationGraph, Display, TEXT( "CLIENTSTREAMING Removing %s from AlwaysRelevantStreamingLevelActors because FActorRepListRefView is null. %s " ), *StreamingLevel.ToString( ), *Params.ConnectionManager.GetName( ) );
            AlwaysRelevantStreamingLevelsNeedingReplication.RemoveAtSwap( Idx, 1, false );
            continue;
        }

        FActorRepListRefView& RepList = *Ptr;

        if ( RepList.Num( ) > 0 )
        {
            bool bAllDormant = true;
            for ( FActorRepListType Actor : RepList )
            {
                FConnectionReplicationActorInfo& ConnectionActorInfo = ConnectionActorInfoMap.FindOrAdd( Actor );
                if ( ConnectionActorInfo.bDormantOnConnection == false )
                {
                    bAllDormant = false;
                    break;
                }
            }

            if ( bAllDormant )
            {
                UE_CLOG( CVar_ShooterRepGraph_DisplayClientLevelStreaming > 0, LogHexReplicationGraph, Display, TEXT( "CLIENTSTREAMING All AlwaysRelevant Actors Dormant on StreamingLevel %s for %s. Removing list." ), *StreamingLevel.ToString( ), *Params.ConnectionManager.GetName( ) );
                AlwaysRelevantStreamingLevelsNeedingReplication.RemoveAtSwap( Idx, 1, false );
            }
            else
            {
                UE_CLOG( CVar_ShooterRepGraph_DisplayClientLevelStreaming > 0, LogHexReplicationGraph, Display, TEXT( "CLIENTSTREAMING Adding always Actors on StreamingLevel %s for %s because it has at least one non dormant actor" ), *StreamingLevel.ToString( ), *Params.ConnectionManager.GetName( ) );
                Params.OutGatheredReplicationLists.AddReplicationActorList( RepList );
            }
        }
        else
        {
            UE_LOG( LogHexReplicationGraph, Warning, TEXT( "UHexReplicationGraphNode_AlwaysRelevant_ForConnection::GatherActorListsForConnection - empty RepList %s" ), *Params.ConnectionManager.GetName( ) );
        }

    }

#if WITH_GAMEPLAY_DEBUGGER
    if ( GameplayDebugger )
    {
        ReplicationActorList.ConditionalAdd( GameplayDebugger );
    }
#endif
}

void UHexReplicationGraphNode_AlwaysRelevant_ForConnection::OnClientLevelVisibilityAdd( FName LevelName, UWorld* StreamingWorld )
{
    UE_CLOG( CVar_ShooterRepGraph_DisplayClientLevelStreaming > 0, LogHexReplicationGraph, Display, TEXT( "CLIENTSTREAMING ::OnClientLevelVisibilityAdd - %s" ), *LevelName.ToString( ) );
    AlwaysRelevantStreamingLevelsNeedingReplication.Add( LevelName );
}

void UHexReplicationGraphNode_AlwaysRelevant_ForConnection::OnClientLevelVisibilityRemove( FName LevelName )
{
    UE_CLOG( CVar_ShooterRepGraph_DisplayClientLevelStreaming > 0, LogHexReplicationGraph, Display, TEXT( "CLIENTSTREAMING ::OnClientLevelVisibilityRemove - %s" ), *LevelName.ToString( ) );
    AlwaysRelevantStreamingLevelsNeedingReplication.Remove( LevelName );
}

void UHexReplicationGraphNode_AlwaysRelevant_ForConnection::LogNode( FReplicationGraphDebugInfo& DebugInfo, const FString& NodeName ) const
{
    DebugInfo.Log( NodeName );
    DebugInfo.PushIndent( );
    LogActorRepList( DebugInfo, NodeName, ReplicationActorList );

    for ( const FName& LevelName : AlwaysRelevantStreamingLevelsNeedingReplication )
    {
        UHexReplicationGraph* ShooterGraph = CastChecked<UHexReplicationGraph>( GetOuter( ) );
        if ( FActorRepListRefView* RepList = ShooterGraph->AlwaysRelevantStreamingLevelActors.Find( LevelName ) )
        {
            LogActorRepList( DebugInfo, FString::Printf( TEXT( "AlwaysRelevant StreamingLevel List: %s" ), *LevelName.ToString( ) ), *RepList );
        }
    }

    DebugInfo.PopIndent( );
}

// ------------------------------------------------------------------------------

UHexReplicationGraphNode_PlayerStateFrequencyLimiter::UHexReplicationGraphNode_PlayerStateFrequencyLimiter( )
{
    bRequiresPrepareForReplicationCall = true;
}

void UHexReplicationGraphNode_PlayerStateFrequencyLimiter::PrepareForReplication( )
{
    QUICK_SCOPE_CYCLE_COUNTER( UHexReplicationGraphNode_PlayerStateFrequencyLimiter_GlobalPrepareForReplication );

    ReplicationActorLists.Reset( );
    ForceNetUpdateReplicationActorList.Reset( );

    ReplicationActorLists.AddDefaulted( );
    FActorRepListRefView* CurrentList = &ReplicationActorLists[ 0 ];
    CurrentList->PrepareForWrite( );

    // We rebuild our lists of player states each frame. This is not as efficient as it could be but its the simplest way
    // to handle players disconnecting and keeping the lists compact. If the lists were persistent we would need to defrag them as players left.

    for ( TActorIterator<APlayerState> It( GetWorld( ) ); It; ++It )
    {
        APlayerState* PS = *It;
        if ( IsActorValidForReplicationGather( PS ) == false )
        {
            continue;
        }

        if ( CurrentList->Num( ) >= TargetActorsPerFrame )
        {
            ReplicationActorLists.AddDefaulted( );
            CurrentList = &ReplicationActorLists.Last( );
            CurrentList->PrepareForWrite( );
        }

        CurrentList->Add( PS );
    }
}

void UHexReplicationGraphNode_PlayerStateFrequencyLimiter::GatherActorListsForConnection( const FConnectionGatherActorListParameters& Params )
{
    const int32 ListIdx = Params.ReplicationFrameNum % ReplicationActorLists.Num( );
    Params.OutGatheredReplicationLists.AddReplicationActorList( ReplicationActorLists[ ListIdx ] );

    if ( ForceNetUpdateReplicationActorList.Num( ) > 0 )
    {
        Params.OutGatheredReplicationLists.AddReplicationActorList( ForceNetUpdateReplicationActorList );
    }
}

void UHexReplicationGraphNode_PlayerStateFrequencyLimiter::LogNode( FReplicationGraphDebugInfo& DebugInfo, const FString& NodeName ) const
{
    DebugInfo.Log( NodeName );
    DebugInfo.PushIndent( );

    int32 i = 0;
    for ( const FActorRepListRefView& List : ReplicationActorLists )
    {
        LogActorRepList( DebugInfo, FString::Printf( TEXT( "Bucket[%d]" ), i++ ), List );
    }

    DebugInfo.PopIndent( );
}

// ------------------------------------------------------------------------------

void UHexReplicationGraph::PrintRepNodePolicies( )
{
    UEnum* Enum = StaticEnum<EClassRepNodeMapping>( );
    if ( !Enum )
    {
        return;
    }

    GLog->Logf( TEXT( "====================================" ) );
    GLog->Logf( TEXT( "Shooter Replication Routing Policies" ) );
    GLog->Logf( TEXT( "====================================" ) );

    for ( auto It = ClassRepNodePolicies.CreateIterator( ); It; ++It )
    {
        FObjectKey ObjKey = It.Key( );

        EClassRepNodeMapping Mapping = It.Value( );

        GLog->Logf( TEXT( "%-40s --> %s" ), *GetNameSafe( ObjKey.ResolveObjectPtr( ) ), *Enum->GetNameStringByValue( static_cast< uint32 >( Mapping ) ) );
    }
}

void UHexReplicationGraph::SetRepDriverWorld( UWorld* InWorld )
{
    Super::SetRepDriverWorld( InWorld );

    //NOTE: Fix network throttling in the PIE
    if ( GEngine && InWorld )
        GEngine->Exec( InWorld, TEXT( "Net.RepGraph.Frequency.MatchTargetInPIE 0" ) );
}

FAutoConsoleCommandWithWorldAndArgs ShooterPrintRepNodePoliciesCmd( TEXT( "ShooterRepGraph.PrintRouting" ), TEXT( "Prints how actor classes are routed to RepGraph nodes" ),
                                                                    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda( [ ]( const TArray<FString>& Args, UWorld* World )
{
    for ( TObjectIterator<UHexReplicationGraph> It; It; ++It )
    {
        It->PrintRepNodePolicies( );
    }
} )
);

// ------------------------------------------------------------------------------

FAutoConsoleCommandWithWorldAndArgs ChangeFrequencyBucketsCmd( TEXT( "ShooterRepGraph.FrequencyBuckets" ), TEXT( "Resets frequency bucket count." ), FConsoleCommandWithWorldAndArgsDelegate::CreateLambda( [ ]( const TArray< FString >& Args, UWorld* World )
{
    int32 Buckets = 1;
    if ( Args.Num( ) > 0 )
    {
        LexTryParseString<int32>( Buckets, *Args[ 0 ] );
    }

    UE_LOG( LogHexReplicationGraph, Display, TEXT( "Setting Frequency Buckets to %d" ), Buckets );
    for ( TObjectIterator<UReplicationGraphNode_ActorListFrequencyBuckets> It; It; ++It )
    {
        UReplicationGraphNode_ActorListFrequencyBuckets* Node = *It;
        Node->SetNonStreamingCollectionSize( Buckets );
    }
} ) );
#endif