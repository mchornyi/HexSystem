// Fill out your copyright notice in the Description page of Project Settings.


#include "HexReplicatorDebugActor.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"

namespace
{
    static float sNetCullDistance = 1000.0f;

    static TAutoConsoleVariable<bool> CVar_ShowDebugInfo(
        TEXT( "hex.HexReplicatorDebugActor.ShowDebugInfo" ),
        true,
        TEXT( "Show debug info of AHexReplicatorDebugActor." ),
        ECVF_Default );
} // namespace

// Sets default values
AHexReplicatorDebugActor::AHexReplicatorDebugActor( )
{
    RootComponent = CreateDefaultSubobject<USceneComponent>( TEXT( "SceneComponent" ) );

    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    bReplicates = true;
    NetCullDistanceSquared = sNetCullDistance * sNetCullDistance;
    NetUpdateFrequency = 2;
}

// Called when the game starts or when spawned
void AHexReplicatorDebugActor::BeginPlay( )
{
    Super::BeginPlay( );
}

// Called every frame
void AHexReplicatorDebugActor::Tick( float DeltaTime )
{
    Super::Tick( DeltaTime );

    //Server-specific functionality
    if ( GetLocalRole( ) == ROLE_Authority )
    {
        OnlineProperty += 0.001f;
    }
    else
    {
        if(GetActorLocation().X == 0)
            DrawDebugSphere( GetWorld( ), GetActorLocation( ), sNetCullDistance, 32, FColor::White, false, -1, 2);
    }
}

//////////////////////////////////////////////////////////////////////////
// Replicated Properties
void AHexReplicatorDebugActor::GetLifetimeReplicatedProps( TArray <FLifetimeProperty>& OutLifetimeProps ) const
{
    Super::GetLifetimeReplicatedProps( OutLifetimeProps );

    //Replicate current health.
    DOREPLIFETIME( AHexReplicatorDebugActor, OnlineProperty );
}

void AHexReplicatorDebugActor::OnRep_OnlineProperty( )
{
    if ( CVar_ShowDebugInfo.GetValueOnGameThread() )
    {
        auto str = FString::Printf( TEXT( "Property1=%.2f" ), OnlineProperty );
        DrawDebugString( GetWorld( ), { 0.0f, 0.0f, 0.0f }, str, this, FColor::Green, 0.1f, false, 1.2f );
    }
}