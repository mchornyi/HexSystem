// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "HexLevelStreamerActor.generated.h"

UCLASS()
class HEXSYSTEM_API AHexLevelStreamerActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AHexLevelStreamerActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION( )
    void OverlapBegins( UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult );

    UFUNCTION( )
    void OverlapEnds( UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex );

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
    UPROPERTY( EditAnywhere )
    FName LevelToLoad;

private:

    UPROPERTY( )
    USceneComponent* SceneComponent;

    // Static Mesh used to provide a visual representation of the object.
    UPROPERTY( )
    class UStaticMeshComponent* StaticMesh;
};
