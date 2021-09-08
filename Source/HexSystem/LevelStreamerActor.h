// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelStreamerActor.generated.h"

class USphereComponent;

UCLASS()
class HEXSYSTEM_API ALevelStreamerActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ALevelStreamerActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OverlapBeginLoad(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void SubOverlapBeginLoad(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void SubOverlapEndUnload(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:

private:

	/** Property replication */
    //void GetLifetimeReplicatedProps( TArray<FLifetimeProperty>& OutLifetimeProps ) const override;

	void LoadLevel();

	// Must be loaded after main level only
	void LoadSubLevel();

	UFUNCTION()
	void OnLevelLoaded();

	UFUNCTION(NetMulticast, Reliable)
	void NotifyLevelLoaded(const FString& InOverrideName, const FVector& InLocation);

	virtual void PostLoad() override;

private:
	// Overlap volume to trigger level streaming
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USphereComponent* OverlapVolume;

	// Overlap volume to trigger sub level streaming
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USphereComponent* SubOverlapVolume;

	UPROPERTY(EditAnywhere)
	FName LevelToLoad;

	UPROPERTY(EditAnywhere)
	FName SubLevelToLoad;

	FString OverrideLevelName;
	FString OverrideSubLevelName;

	FVector LevelLocation;

	bool bIsLevelLoaded;
	bool bIsSubLevelLoaded;
};
