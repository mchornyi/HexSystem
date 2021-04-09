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

    /** Delegate called when a property changes */
    DECLARE_MULTICAST_DELEGATE_OneParam( FOnPropertyChanged, AHexWorld& );

protected:

    UPROPERTY( EditInstanceOnly, Category = "HexWorldParams",  Meta = ( ClampMin = "1", ClampMax = "50" ) )
    int32 RingsCount = 1;

    //UPROPERTY( EditInstanceOnly )
    //TArray<FGeneratedRing> Rings;

    FOnPropertyChanged OnPropertyChanged;
#endif

private:
    void Generate( );

#ifdef WITH_EDITOR
    virtual bool ShouldTickIfViewportsOnly( ) const override;
#endif
    void UpdateHexLayout( );
    //void OnChangeCVar( IConsoleVariable* var );

private:
    TSet<FHexModel::FHex> HexMap;
    FHexModel::FLayout HexLayout;
};
