// Fill out your copyright notice in the Description page of Project Settings.


#include "HexReplicatorDebugDynamicActor.h"

#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "../../CommonCVars.h"

DECLARE_LOG_CATEGORY_EXTERN( HexTrace, Log, All );

// Sets default values
AHexReplicatorDebugDynamicActor::AHexReplicatorDebugDynamicActor( )
: mMovingCenter(ForceInitToZero), mMovingRadius(500.0f), mMovingAngle(0.0f)
{
    RootComponent = CreateDefaultSubobject<USceneComponent>( TEXT( "SceneComponent" ) );

    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    bReplicates = true;
    SetReplicateMovement( true );

    NetCullDistanceSquared = GNetCullDistanceHexRepActor * GNetCullDistanceHexRepActor;
    NetUpdateFrequency = 100;
}

// Called when the game starts or when spawned
void AHexReplicatorDebugDynamicActor::BeginPlay( )
{
    Super::BeginPlay( );

    mMovingCenter = GetActorLocation( );
}

// Called every frame
void AHexReplicatorDebugDynamicActor::Tick( float DeltaTime )
{
    Super::Tick( DeltaTime );

    //Server-specific functionality
    if ( HasAuthority() )
    {
        OnlineProperty += 0.001f;

        mMovingAngle += DeltaTime * 0.5f;
        mMovingAngle = FMath::Clamp( mMovingAngle, 0.0f, 360.0f );

        if ( mMovingAngle > 360 )
            mMovingAngle = 0.0f;

        float dX = FMath::Cos( mMovingAngle ) * mMovingRadius;
        float dY = FMath::Sin( mMovingAngle ) * mMovingRadius;
        dX += mMovingCenter.X;
        dY += mMovingCenter.Y;

        SetActorLocation( { dX, dY, mMovingCenter.Z } );
    }
    else
    {
        //if(GetActorLocation().X == 0)
            DrawDebugSphere( GetWorld( ), GetActorLocation( ), GNetCullDistanceHexRepActor, 64, FColor::Blue, false, -1, 2);
    }
}

//////////////////////////////////////////////////////////////////////////
// Replicated Properties
void AHexReplicatorDebugDynamicActor::GetLifetimeReplicatedProps( TArray <FLifetimeProperty>& OutLifetimeProps ) const
{
    Super::GetLifetimeReplicatedProps( OutLifetimeProps );

    //Replicate current health.
    DOREPLIFETIME( AHexReplicatorDebugDynamicActor, OnlineProperty );
}

void AHexReplicatorDebugDynamicActor::OnRep_OnlineProperty( )
{
    if ( GCVar_DebugShowInfoForHexRepActor.GetValueOnGameThread() )
    {
        auto str = FString::Printf( TEXT( "RepValue=%.2f" ), OnlineProperty );
        DrawDebugString( GetWorld( ), { 0.0f, 0.0f, 0.0f }, str, this, FColor::Green, 0.1f, false, 1.2f );
    }
}