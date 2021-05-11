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

    // Called every frame
    virtual void Tick(float DeltaTime) override;

    void SetOnlinePropertyPushModel(float value);

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

private:
    /** Property replication */
    void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION()
    void OnRep_OnlineProperty();

    UFUNCTION()
    void OnRep_OnlinePropertyPushModel();

private:
    /** The player's current health. When reduced to 0, they are considered dead.*/
    UPROPERTY(ReplicatedUsing = OnRep_OnlineProperty)
    float OnlineProperty;

    UPROPERTY(ReplicatedUsing = OnRep_OnlinePropertyPushModel)
    float OnlinePropertyPushModel;

    FVector MovingCenter;
    float MovingRadius;
    float MovingAngle;
};
