// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HexReplicatorDebugDormantActor.generated.h"

UCLASS()
class HEXSYSTEM_API AHexReplicatorDebugDormantActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AHexReplicatorDebugDormantActor();

    void FlushNetDormancyOnce();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// Called every frame
	virtual void Tick(float DeltaTime) override;
    /** Returns true if the actor should be dormant for a specific net connection. Only checked for DORM_DormantPartial */
	virtual bool GetNetDormancy(const FVector& ViewPos, const FVector& ViewDir, class AActor* Viewer, AActor* ViewTarget, UActorChannel* InChannel, float Time, bool bLowBandwidth);
 public:
    /** The player's current health. When reduced to 0, they are considered dead.*/
    UPROPERTY( ReplicatedUsing = OnRep_OnlineProperty )
    float OnlineProperty;

private:
    /** Property replication */
    virtual void GetLifetimeReplicatedProps( TArray<FLifetimeProperty>& OutLifetimeProps ) const override;

    /** RepNotify for changes made to current health.*/
    UFUNCTION( )
    void OnRep_OnlineProperty( );

    void FlushNetDormancyInternal();
};
