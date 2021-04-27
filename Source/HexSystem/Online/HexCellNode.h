#pragma once

#include "ReplicationGraph.h"
#include "../HexModel.h"

#include "HexDormancyNode.h"

#include "HexCellNode.generated.h"

class UReplicationGraphNode_HexDormancyNode;

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

    // Allow graph to override function for creating the dynamic node in the cell
    TFunction<UReplicationGraphNode* ( UReplicationGraphNode_HexCell* Parent )> CreateDynamicNodeOverride;

    void SetHex( FHex hex )
    {
        check( mHex.IsZero( ) );
        mHex = hex;
    }

    const FHex& GetHex( ) const
    {
        return mHex;
    }

    UReplicationGraphNode_HexDormancyNode* GetDormancyNode( )
    {
        if ( DormancyNode != nullptr )
            return DormancyNode;

        DormancyNode = CreateChildNode<UReplicationGraphNode_HexDormancyNode>( );

        return DormancyNode;
    }

private:

    UPROPERTY( )
    UReplicationGraphNode* DynamicNode = nullptr;

    UPROPERTY( )
    UReplicationGraphNode_HexDormancyNode* DormancyNode = nullptr;

    UReplicationGraphNode* GetDynamicNode( )
    {
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

    void OnActorDormancyFlush( FActorRepListType Actor, FGlobalActorReplicationInfo& GlobalInfo, UReplicationGraphNode_DormancyNode* DormancyNode );

    void ConditionalCopyDormantActors( FActorRepListRefView& FromList, UReplicationGraphNode_DormancyNode* ToNode ) const;
    void OnStaticActorNetDormancyChange( FActorRepListType Actor, FGlobalActorReplicationInfo& GlobalInfo, ENetDormancy NewVlue, ENetDormancy OldValue );

private:
    FHex mHex;
};