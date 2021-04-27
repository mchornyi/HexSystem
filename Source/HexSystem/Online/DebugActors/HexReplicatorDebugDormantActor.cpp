// Fill out your copyright notice in the Description page of Project Settings.


#include "HexReplicatorDebugDormantActor.h"

#include "../../CommonCVars.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AHexReplicatorDebugDormantActor::AHexReplicatorDebugDormantActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    bReplicates = true;
	NetCullDistanceSquared = Global_NetCullDistanceHexRepActor * Global_NetCullDistanceHexRepActor;
	NetUpdateFrequency = 2;
	NetDormancy = DORM_DormantAll;
}

// Called when the game starts or when spawned
void AHexReplicatorDebugDormantActor::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AHexReplicatorDebugDormantActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    //Server-specific functionality
    if ( HasAuthority( ) )
    {
        OnlineProperty += 0.001f;

        if ( Global_DebugEnableFlushForDormantActor )
            FlushNetDormancy( );
    }
    else
    {
        //if(GetActorLocation().X == 0)
            //DrawDebugSphere( GetWorld( ), GetActorLocation( ), Global_NetCullDistanceHexRepActor, 32, FColor::White, false, -1, 2);
    }
}

//////////////////////////////////////////////////////////////////////////
// Replicated Properties
void AHexReplicatorDebugDormantActor::GetLifetimeReplicatedProps( TArray <FLifetimeProperty>& OutLifetimeProps ) const
{
    Super::GetLifetimeReplicatedProps( OutLifetimeProps );

    //Replicate current health.
    DOREPLIFETIME( AHexReplicatorDebugDormantActor, OnlineProperty );
}

void AHexReplicatorDebugDormantActor::OnRep_OnlineProperty( )
{
    if ( CVar_DebugShowInfoForHexRepActor.GetValueOnGameThread( ) )
    {
        const auto str = FString::Printf( TEXT( "Dormant RepValue=%.2f" ), OnlineProperty );
        DrawDebugString( GetWorld( ), { 0.0f, 0.0f, 0.0f }, str, this, FColor::Orange, 0.1f, false, 1.2f );
    }
}
