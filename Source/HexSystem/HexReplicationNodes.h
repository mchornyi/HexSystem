#pragma once

#include "ReplicationGraph.h"
#include "ReplicationGraphTypes.h"
#include "HexModel.h"

#include "HexReplicationNodes.generated.h"

// -----------------------------------

UCLASS( )
class HEXSYSTEM_API UReplicationGraphNode_HexCell : public UReplicationGraphNode_ActorList
{
    GENERATED_BODY( )

public:

    virtual void NotifyAddNetworkActor( const FNewReplicatedActorInfo& ActorInfo ) override
    {
        ensureMsgf( false, TEXT( "UReplicationGraphNode_Simple2DSpatializationLeaf::NotifyAddNetworkActor not functional." ) );
    }

    virtual bool NotifyRemoveNetworkActor( const FNewReplicatedActorInfo& ActorInfo, bool bWarnIfNotFound = true ) override
    {
        ensureMsgf( false, TEXT( "UReplicationGraphNode_Simple2DSpatializationLeaf::NotifyRemoveNetworkActor not functional." ) ); return false;
    }

    virtual void GetAllActorsInNode_Debugging( TArray<FActorRepListType>& OutArray ) const override;

    void AddStaticActor( const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& GlobalRepInfo, bool bParentNodeHandlesDormancyChange );
    void AddDynamicActor( const FNewReplicatedActorInfo& ActorInfo );

    void RemoveStaticActor( const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& ActorRepInfo, bool bWasAddedAsDormantActor );
    void RemoveDynamicActor( const FNewReplicatedActorInfo& ActorInfo );

    // Allow graph to override function for creating the dynamic node in the cell
    TFunction<UReplicationGraphNode* ( UReplicationGraphNode_HexCell* Parent )> CreateDynamicNodeOverride;

    UReplicationGraphNode_DormancyNode* GetDormancyNode( );

    void SetHex( FHex hex )
    {
        check( mHex.IsZero( ) );
        mHex = hex;
    }

    const FHex& GetHex( ) const
    {
        return mHex;
    }

private:

    UPROPERTY( )
        UReplicationGraphNode* DynamicNode = nullptr;

    UPROPERTY( )
        UReplicationGraphNode_DormancyNode* DormancyNode = nullptr;

    UReplicationGraphNode* GetDynamicNode( );

    void OnActorDormancyFlush( FActorRepListType Actor, FGlobalActorReplicationInfo& GlobalInfo, UReplicationGraphNode_DormancyNode* DormancyNode );

    void ConditionalCopyDormantActors( FActorRepListRefView& FromList, UReplicationGraphNode_DormancyNode* ToNode ) const;
    void OnStaticActorNetDormancyChange( FActorRepListType Actor, FGlobalActorReplicationInfo& GlobalInfo, ENetDormancy NewVlue, ENetDormancy OldValue );

private:
    FHex mHex;
};

// -----------------------------------

UCLASS( )
class HEXSYSTEM_API UReplicationGraphNode_HexSpatialization2D : public UReplicationGraphNode
{
    GENERATED_BODY( )

    friend class AReplicationGraphDebugActor;
    friend class FHexReplicationGraphTest;

public:

    UReplicationGraphNode_HexSpatialization2D( );

    virtual void NotifyAddNetworkActor( const FNewReplicatedActorInfo& Actor ) override;
    virtual bool NotifyRemoveNetworkActor( const FNewReplicatedActorInfo& ActorInfo, bool bWarnIfNotFound = true ) override;
    virtual void NotifyResetAllNetworkActors( ) override;
    virtual void PrepareForReplication( ) override;
    virtual void GatherActorListsForConnection( const FConnectionGatherActorListParameters& Params ) override;

    void AddActor_Static( const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& ActorRepInfo )
    {
        AddActorInternal_Static( ActorInfo, ActorRepInfo, false );
    }

    void AddActor_Dynamic( const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& ActorRepInfo )
    {
        AddActorInternal_Dynamic( ActorInfo );
    }

    void AddActor_Dormancy( const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& ActorRepInfo );

    void RemoveActor_Static( const FNewReplicatedActorInfo& ActorInfo );

    void RemoveActor_Dynamic( const FNewReplicatedActorInfo& ActorInfo )
    {
        RemoveActorInternal_Dynamic( ActorInfo );
    }

    void RemoveActor_Dormancy( const FNewReplicatedActorInfo& ActorInfo );

    // Called if cull distance changes. Note the caller must update Global/Connection actor rep infos. This just changes cached state within this node
    void NotifyActorCullDistChange( AActor* actor, FGlobalActorReplicationInfo& actorGlobalInfo, float oldDist );

    /** When the mGridBounds is set we limit the creation of cells to be exclusively inside the passed region.
        Viewers who gather nodes outside this region will be clamped to the closest cell inside the box.
        Actors whose location is outside the box will be clamped to the closest cell inside the box.
    */
    void AddSpatialRebuildBlacklistClass( UClass* Class );

protected:

    /** For adding new actor to the graph */
    virtual void AddActorInternal_Dynamic( const FNewReplicatedActorInfo& ActorInfo );
    virtual void AddActorInternal_Static( const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& ActorRepInfo, bool IsDormancyDriven ); // Can differ the actual call to implementation if actor is not ready
    virtual void AddActorInternal_Static_Implementation( const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& ActorRepInfo, bool IsDormancyDriven );

    virtual void RemoveActorInternal_Dynamic( const FNewReplicatedActorInfo& Actor );
    virtual void RemoveActorInternal_Static( const FNewReplicatedActorInfo& Actor );

private:

    void OnNetDormancyChange( FActorRepListType Actor, FGlobalActorReplicationInfo& GlobalInfo, ENetDormancy NewVlue, ENetDormancy OldValue );

    void AddStaticActorIntoCell( const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& ActorRepInfo, bool bDormancyDriven );

    UReplicationGraphNode_HexCell* CreateHexCellNode( UReplicationGraphNode_HexCell*& hexCellPtr, const FHex& hex );

    TArray<UReplicationGraphNode_HexCell*> GetHexCellCoverage( const FVector& location, const float cullDistance );

    void RepGraphVerifySlow( const FActorRepListType& actor, FGlobalActorReplicationInfo& actorRepInfo, bool mustExist );

private:

    struct FDynamicActorInfo
    {
        FDynamicActorInfo( const FNewReplicatedActorInfo& inInfo ) : actorInfo( inInfo ), prevLocation(NAN, NAN )
        {
        }

        FNewReplicatedActorInfo actorInfo;
        FVector2D prevLocation;
    };

    struct FStaticActorInfo
    {
        FStaticActorInfo( const FNewReplicatedActorInfo& inInfo, bool bInDormDriven ) : actorInfo( inInfo ), bDormancyDriven( bInDormDriven )
        {
        }

        FNewReplicatedActorInfo actorInfo;
        bool bDormancyDriven = false; // This actor will be removed from static actor list if it becomes non dormant.
    };

    // Static spatialized actors that were not fully initialized when registering with this node. We differ their placement in the grid until the next frame
    struct FPendingStaticActors
    {
        FPendingStaticActors( const FActorRepListType& inActor, const bool InDormancyDriven ) : actor( inActor ), bDormancyDriven( InDormancyDriven )
        {
        }

        bool operator==( const FActorRepListType& inActor ) const
        {
            return inActor == actor;
        }

        FActorRepListType actor;
        bool bDormancyDriven;
    };

    float mConnectionMaxZ = WORLD_MAX; // Connection locations have to be <= to this to pull from the grid

    // When enabled the RepGraph tells clients to destroy dormant dynamic actors when they go out of relevancy.
    bool bDestroyDormantDynamicActors = true;

    // Classmap of actor classes which CANNOT force a rebuild of the spatialization tree. They will be clamped instead. E.g, projectiles.
    TClassMap<bool> mRebuildSpatialBlacklistMap;

    TMap<FActorRepListType, FDynamicActorInfo> mDynamicSpatializedActors;

    TMap<FActorRepListType, FStaticActorInfo> mStaticSpatializedActors;

    TArray<FPendingStaticActors> mPendingStaticSpatializedActors;

    TMap<FHex, UReplicationGraphNode_HexCell*> mHexCells;

    // Allow graph to override function for creating cell nodes in this grid.
    TFunction<UReplicationGraphNode_HexCell* ( UReplicationGraphNode_HexSpatialization2D* Parent )>	mCreateCellNodeOverrideFun;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
    TArray<FString> mDebugActorNames;
#endif
};