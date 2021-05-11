#pragma once

#include "ReplicationGraph.h"

#include "../HexModel.h"

#include "HexCellNode.generated.h"

//class UReplicationGraphNode_HexDormancyNode;

UCLASS( )
class HEXSYSTEM_API UReplicationGraphNode_HexCell : public UReplicationGraphNode_ActorList
{
    GENERATED_BODY( )

public:

    UReplicationGraphNode_HexCell( );

    virtual void NotifyAddNetworkActor( const FNewReplicatedActorInfo& ActorInfo ) override
    {
        ensureMsgf( false, TEXT( "UReplicationGraphNode_Simple2DSpatializationLeaf::NotifyAddNetworkActor not functional." ) );
    }

    virtual bool NotifyRemoveNetworkActor( const FNewReplicatedActorInfo& ActorInfo, bool bWarnIfNotFound = true ) override
    {
        ensureMsgf( false, TEXT( "UReplicationGraphNode_Simple2DSpatializationLeaf::NotifyRemoveNetworkActor not functional." ) );

        return false;
    }

    virtual void GetAllActorsInNode_Debugging( TArray<FActorRepListType>& OutArray ) const override;

    void AddStaticActor( const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& GlobalRepInfo, bool bParentNodeHandlesDormancyChange );

    void AddDynamicActor( const FNewReplicatedActorInfo& ActorInfo );

    void RemoveStaticActor( const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& ActorRepInfo, bool bWasAddedAsDormantActor );

    void RemoveDynamicActor( const FNewReplicatedActorInfo& ActorInfo );

    void SetHex( FHex hex )
    {
        check( mHex.IsZero( ) );
        mHex = hex;
    }

    const FHex& GetHex( ) const
    {
        return mHex;
    }

    UReplicationGraphNode_DormancyNode* GetDormancyNode( );

private:

    UPROPERTY( )
    UReplicationGraphNode* DynamicNode = nullptr;

    UPROPERTY( )
    UReplicationGraphNode_DormancyNode* DormancyNode = nullptr;

    UReplicationGraphNode* GetDynamicNode( );

    void ConditionalCopyDormantActors( FActorRepListRefView& FromList, UReplicationGraphNode_DormancyNode* ToNode ) const;
    void OnStaticActorNetDormancyChange( FActorRepListType Actor, FGlobalActorReplicationInfo& GlobalInfo, ENetDormancy NewVlue, ENetDormancy OldValue );

private:
    FHex mHex;
};