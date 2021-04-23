// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HexReplicatorDebugDynamicActor.generated.h"

UCLASS()
class HEXSYSTEM_API AHexReplicatorDebugDynamicActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
    AHexReplicatorDebugDynamicActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
    /** The player's current health. When reduced to 0, they are considered dead.*/
    UPROPERTY( ReplicatedUsing = OnRep_OnlineProperty )
    float OnlineProperty;

private:
    /** Property replication */
    void GetLifetimeReplicatedProps( TArray<FLifetimeProperty>& OutLifetimeProps ) const override;

    /** RepNotify for changes made to current health.*/
    UFUNCTION( )
    void OnRep_OnlineProperty( );
private:

    FVector mMovingCenter;
    float mMovingRadius;
    float mMovingAngle;
};
