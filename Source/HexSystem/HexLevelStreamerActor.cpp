// Fill out your copyright notice in the Description page of Project Settings.


#include "HexLevelStreamerActor.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

DECLARE_LOG_CATEGORY_EXTERN( HexTrace, Log, All );

namespace
{
    const TCHAR* sHexTileSimpleLarge = TEXT( "/Game/ThirdPersonCPP/HexTile/Simple/HexTileSimpleLarge.HexTileSimpleLarge" );
}

// Sets default values
AHexLevelStreamerActor::AHexLevelStreamerActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	//PrimaryActorTick.bCanEverTick = true;

    SceneComponent = CreateDefaultSubobject<USceneComponent>( TEXT( "SceneComponent" ) );
    RootComponent = SceneComponent;

    //Definition for the Mesh that will serve as our visual representation.
    StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>( TEXT( "HexSimpleMesh" ) );
    StaticMesh->SetupAttachment( RootComponent );

    //Set the Static Mesh and its position/scale if we successfully found a mesh asset to use.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMesh( sHexTileSimpleLarge );
    if ( DefaultMesh.Succeeded( ) )
    {
        StaticMesh->SetStaticMesh( DefaultMesh.Object );
    }

    for ( TObjectIterator<UShapeComponent> shapeItr; shapeItr; ++shapeItr )
    {
        if ( shapeItr->GetName() == "LevelStreamingVolume" )
        {
            shapeItr->OnComponentBeginOverlap.AddUniqueDynamic( this, &AHexLevelStreamerActor::OverlapBegins );
            shapeItr->OnComponentEndOverlap.AddUniqueDynamic( this, &AHexLevelStreamerActor::OverlapEnds );
            break;
        }
    }
}

// Called when the game starts or when spawned
void AHexLevelStreamerActor::BeginPlay()
{
	Super::BeginPlay();

}

void AHexLevelStreamerActor::OverlapBegins( UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult )
{
    UE_LOG( HexTrace, Display, TEXT( __FUNCTION__ ) );
    ACharacter* MyCharacter = UGameplayStatics::GetPlayerCharacter( this, 0 );

    if ( OtherActor == Cast<AActor>(MyCharacter) && LevelToLoad != "" )
    {
        FLatentActionInfo LatentInfo;
        UGameplayStatics::LoadStreamLevel( this, LevelToLoad, true, true, LatentInfo );
    }
}

void AHexLevelStreamerActor::OverlapEnds( UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex )
{
    UE_LOG( HexTrace, Display, TEXT( __FUNCTION__ ) );
    return;
    /*ACharacter* MyCharacter = UGameplayStatics::GetPlayerCharacter( this, 0 );
    if ( OtherActor == MyCharacter && LevelToLoad != "" )
    {
        FLatentActionInfo LatentInfo;
        UGameplayStatics::UnloadStreamLevel( this, LevelToLoad, LatentInfo );
    }*/
}

// Called every frame
void AHexLevelStreamerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

