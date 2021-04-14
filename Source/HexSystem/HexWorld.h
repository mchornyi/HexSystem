// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HexModel.h"
#include "HexWorld.generated.h"

UCLASS()
class HEXSYSTEM_API AHexWorld : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AHexWorld();

    // Called every frame
    virtual void Tick( float DeltaTime ) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

#if WITH_EDITORONLY_DATA
    virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent ) override;
#endif

private:
    void Generate( );

#ifdef WITH_EDITOR
    virtual bool ShouldTickIfViewportsOnly( ) const override;
#endif
    void DebugGenerateRepActors( );

public:
    UPROPERTY( EditInstanceOnly, Category = "HexWorldParams", Meta = ( ClampMin = "1", ClampMax = "100" ) )
    FHexLayout HexLayout;

    UPROPERTY( EditInstanceOnly, Category = "HexWorldParams", Meta = ( ClampMin = "1", ClampMax = "100" ) )
    uint8 HexRingsNum;

    UPROPERTY( EditInstanceOnly, Category = "HexWorldParams", Meta = ( ClampMin = "0", ClampMax = "1000000" ) )
    uint8 HexDistVisibility;

    UPROPERTY( EditInstanceOnly, Category = "HexWorldParams" )
    bool bHexGenerateTrigger;

private:
    UPROPERTY( EditInstanceOnly, Category = "HexWorldParams" )
    TSet<FHex> HexMap;
};
