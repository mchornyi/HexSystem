// Fill out your copyright notice in the Description page of Project Settings.


#include "HexReplicatorDebugDormantActor.h"

#include "Net/UnrealNetwork.h"

#include "../../CommonCVars.h"
#include "DrawDebugHelpers.h"

static bool bUpdatePropertyInTick = false;
static bool bEnableFlushingInTick = false;
static bool GetNetDormancyDefaultValue = false;
static float LableLifeTimeSec = 1.0f;

// Sets default values
AHexReplicatorDebugDormantActor::AHexReplicatorDebugDormantActor()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    bReplicates = true;
    NetCullDistanceSquared = GNetCullDistanceHexRepActor * GNetCullDistanceHexRepActor;
    NetUpdateFrequency = 1.0f/5.0f;// 1 per 5 sec
    //NetDormancy = DORM_DormantAll;
    NetDormancy = DORM_Initial;
    //NetDormancy = DORM_DormantPartial; // Not working for rep graph
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
    if(HasAuthority())
    {
        if(bUpdatePropertyInTick)
            OnlineProperty += 0.001f;

        if(bEnableFlushingInTick)
            FlushNetDormancyInternal();
    }
    /*else
    {
        if(GetActorLocation().X == 0)
            DrawDebugSphere( GetWorld( ), GetActorLocation( ), GNetCullDistanceHexRepActor, 32, FColor::White, false, -1, 2);
    }*/
}

//////////////////////////////////////////////////////////////////////////
// Replicated Properties
void AHexReplicatorDebugDormantActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    //Replicate current health.
    DOREPLIFETIME(AHexReplicatorDebugDormantActor, OnlineProperty);
}

void AHexReplicatorDebugDormantActor::OnRep_OnlineProperty()
{
    if(GCVar_DebugShowInfoForHexRepActor.GetValueOnGameThread())
    {
        const auto str = FString::Printf(TEXT("AHexReplicatorDebugDormantActor::OnlineProperty=%.2f"), OnlineProperty);
        static FVector location{0.0f, 0.0f, 0.0f};
        location.Z += 20;
        if(location.Z > 100) location.Z = 0;
        DrawDebugString(GetWorld(), location, str, this, FColor::Green, LableLifeTimeSec, false, 1.6f);
    }
}

void AHexReplicatorDebugDormantActor::FlushNetDormancyInternal()
{
    FlushNetDormancy();
}

void AHexReplicatorDebugDormantActor::FlushNetDormancyOnce()
{
    check(HasAuthority());

    static float value = 0.0f;

    OnlineProperty = ++value;

    UE_LOG(LogHexSystem, Display, TEXT("Server AHexReplicatorDebugDormantActor::OnlineProperty is %.4f"), OnlineProperty);

    FlushNetDormancyInternal();
}

bool AHexReplicatorDebugDormantActor::GetNetDormancy(const FVector& ViewPos, const FVector& ViewDir, AActor* Viewer, AActor* ViewTarget, UActorChannel* InChannel, float Time, bool bLowBandwidth)
{
    return GetNetDormancyDefaultValue;
}