#pragma once

#include "ReplicationGraph.h"
//#include "ReplicationGraphTypes.h"

#include "HexDormancyNode.generated.h"

struct FHexStreamingLevelActorListCollection
{
	void AddActor( const FNewReplicatedActorInfo& ActorInfo );
	bool RemoveActor( const FNewReplicatedActorInfo& ActorInfo, bool bWarnIfNotFound, UReplicationGraphNode* Outer );
	bool RemoveActorFast( const FNewReplicatedActorInfo& ActorInfo, UReplicationGraphNode* Outer );
	void Reset( );
	void Gather( const FConnectionGatherActorListParameters& Params );
	void DeepCopyFrom( const FHexStreamingLevelActorListCollection& Source );
	void GetAll_Debug( TArray<FActorRepListType>& OutArray ) const;
	void Log( FReplicationGraphDebugInfo& DebugInfo ) const;
	int32 NumLevels( ) const
	{
		return StreamingLevelLists.Num( );
	}

	struct FStreamingLevelActors
	{
		FStreamingLevelActors( FName InName ) : StreamingLevelName( InName )
		{
			repCheck( InName != NAME_None ); ReplicationActorList.Reset( 4 ); /** FIXME[19]: see above comment */
		}

	    bool operator==( const FName& InName ) const
		{
			return InName == StreamingLevelName;
		};

		FName StreamingLevelName;
		FActorRepListRefView ReplicationActorList;
	};

	/** Lists for streaming levels. Actors that "came from" streaming levels go here. These lists are only returned if the connection has their streaming level loaded. */
	static const int32 NumInlineAllocations = 4;
	TArray<FStreamingLevelActors, TInlineAllocator<NumInlineAllocations>> StreamingLevelLists;

	void CountBytes( FArchive& Ar )
	{
		StreamingLevelLists.CountBytes( Ar );
	}
};

/** Removes dormant (on connection) actors from its rep lists */
UCLASS( )
class HEXSYSTEM_API UReplicationGraphNode_HexConnectionDormancyNode : public UReplicationGraphNode_ActorList
{
	GENERATED_BODY( )

   UReplicationGraphNode_HexConnectionDormancyNode( );

public:
	virtual void GatherActorListsForConnection( const FConnectionGatherActorListParameters& Params ) override;
	virtual bool NotifyRemoveNetworkActor( const FNewReplicatedActorInfo& ActorInfo, bool WarnIfNotFound ) override;
	virtual void NotifyResetAllNetworkActors( ) override;

	void NotifyActorDormancyFlush( FActorRepListType Actor );

	void OnClientVisibleLevelNameAdd( FName LevelName, UWorld* World );
	void OnPostReplicatePrioritizeLists( UNetReplicationGraphConnection*, FPrioritizedRepList* );

private:
	void ConditionalGatherDormantActorsForConnection( FActorRepListRefView& ConnectionRepList, const FConnectionGatherActorListParameters& Params, FActorRepListRefView* RemovedList );

private:
	int32 TrickleStartCounter = 10;

	// Tracks actors we've removed in this per-connection node, so that we can restore them if the streaming level is unloaded and reloaded.
	FHexStreamingLevelActorListCollection RemovedStreamingLevelActorListCollection;

	FActorRepListRefView FlushedActorList;
};


/** Stores per-connection copies of a master actor list. Skips and removes elements from per connection list that are fully dormant */
UCLASS( )
class HEXSYSTEM_API UReplicationGraphNode_HexDormancyNode : public UReplicationGraphNode_ActorList
{
	GENERATED_BODY( )

public:

	static float MaxZForConnection; // Connection Z location has to be < this for ConnectionsNodes to be made.

	virtual void NotifyAddNetworkActor( const FNewReplicatedActorInfo& ActorInfo ) override
	{
		ensureMsgf( false, TEXT( "UReplicationGraphNode_DormancyNode::NotifyAddNetworkActor not functional." ) );
	}
	virtual bool NotifyRemoveNetworkActor( const FNewReplicatedActorInfo& ActorInfo, bool bWarnIfNotFound = true ) override
	{
		ensureMsgf( false, TEXT( "UReplicationGraphNode_DormancyNode::NotifyRemoveNetworkActor not functional." ) ); return false;
	}
	virtual void NotifyResetAllNetworkActors( ) override;

	void AddDormantActor( const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& GlobalInfo );
	void RemoveDormantActor( const FNewReplicatedActorInfo& ActorInfo, FGlobalActorReplicationInfo& ActorRepInfo );

	virtual void GatherActorListsForConnection( const FConnectionGatherActorListParameters& Params ) override;

	void OnActorDormancyFlush( FActorRepListType Actor, FGlobalActorReplicationInfo& GlobalInfo );

	void ConditionalGatherDormantDynamicActors( FActorRepListRefView& RepList, const FConnectionGatherActorListParameters& Params, FActorRepListRefView* RemovedList, bool bEnforceReplistUniqueness = false );

	UReplicationGraphNode_HexConnectionDormancyNode* GetExistingConnectionNode( const FConnectionGatherActorListParameters& Params );

	UReplicationGraphNode_HexConnectionDormancyNode* GetConnectionNode( const FConnectionGatherActorListParameters& Params );

private:

	/** Function called on every ConnectionDormancyNode in our list */
	typedef TFunction<void( UReplicationGraphNode_HexConnectionDormancyNode* )> FConnectionDormancyNodeFunction;

	/**
	 * Iterates over all ConnectionDormancyNodes and calls the function on those still valid.
	 * If a RepGraphConnection was torn down since the last iteration, it removes and destroys the ConnectionDormancyNode associated with the Connection.
	 */
	void CallFunctionOnValidConnectionNodes( FConnectionDormancyNodeFunction Function );

private:

	typedef TObjectKey<UNetReplicationGraphConnection> FRepGraphConnectionKey;
	typedef TSortedMap<FRepGraphConnectionKey, UReplicationGraphNode_HexConnectionDormancyNode*> FConnectionDormancyNodeMap;
	FConnectionDormancyNodeMap ConnectionNodes;
};
