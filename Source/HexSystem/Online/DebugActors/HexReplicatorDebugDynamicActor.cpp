// Fill out your copyright notice in the Description page of Project Settings.


#include "HexReplicatorDebugDynamicActor.h"

#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "../../CommonCVars.h"
#include "Net/Core/PushModel/PushModel.h"

DECLARE_LOG_CATEGORY_EXTERN(HexTrace, Log, All);

// Sets default values
AHexReplicatorDebugDynamicActor::AHexReplicatorDebugDynamicActor()
    : OnlineProperty(0.0f)
      , OnlinePropertyPushModel(0.0f)
      , MovingCenter(ForceInitToZero)
      , MovingRadius(500.0f)
      , MovingAngle(0.0f)
{
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));

    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

    bReplicates = true;
    SetReplicateMovement(true);

    NetCullDistanceSquared = GNetCullDistanceHexRepActor * GNetCullDistanceHexRepActor;
    NetUpdateFrequency = 100;
}

// Called when the game starts or when spawned
void AHexReplicatorDebugDynamicActor::BeginPlay()
{
    Super::BeginPlay();

    MovingCenter = GetActorLocation();
}

// Called every frame
void AHexReplicatorDebugDynamicActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    //Server-specific functionality
    if(HasAuthority())
    {
        OnlineProperty += 0.001f;

        MovingAngle += DeltaTime * 0.5f;
        MovingAngle = FMath::Clamp(MovingAngle, 0.0f, 360.0f);

        if(MovingAngle > 360)
            MovingAngle = 0.0f;

        float dX = FMath::Cos(MovingAngle) * MovingRadius;
        float dY = FMath::Sin(MovingAngle) * MovingRadius;
        dX += MovingCenter.X;
        dY += MovingCenter.Y;

        SetActorLocation({dX, dY, MovingCenter.Z});

        if(OnlineProperty - OnlinePropertyPushModel > 1.0f)
            SetOnlinePropertyPushModel(OnlineProperty);
    }
    else
    {
        DrawDebugSphere(GetWorld(), GetActorLocation(), GNetCullDistanceHexRepActor, 64, FColor::Blue, false, -1, 2);
    }
}

//////////////////////////////////////////////////////////////////////////
// Replicated Properties
void AHexReplicatorDebugDynamicActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AHexReplicatorDebugDynamicActor, OnlineProperty);

    FDoRepLifetimeParams SharedParams;
    SharedParams.bIsPushBased = true;
    DOREPLIFETIME_WITH_PARAMS_FAST(AHexReplicatorDebugDynamicActor, OnlinePropertyPushModel, SharedParams);
}

void AHexReplicatorDebugDynamicActor::OnRep_OnlineProperty()
{
    if(GCVar_DebugShowInfoForHexRepActor.GetValueOnGameThread())
    {
        auto str = FString::Printf(TEXT("AHexReplicatorDebugDynamicActor::OnlineProperty=%.2f"), OnlineProperty);
        DrawDebugString(GetWorld(), {0.0f, 0.0f, 0.0f}, str, this, FColor::Green, 0.1f, false, 1.2f);
    }
}

void AHexReplicatorDebugDynamicActor::OnRep_OnlinePropertyPushModel()
{
    if(GCVar_DebugShowInfoForHexRepActor.GetValueOnGameThread())
    {
        const auto str = FString::Printf(TEXT("AHexReplicatorDebugDynamicActor::OnlinePropertyPushModel=%.3f"), OnlinePropertyPushModel);
        static FVector location{0.0f, 0.0f, 20.0f};
        DrawDebugString(GetWorld(), location, str, this, FColor::Orange, 5.0f, false, 2.0f);
    }
}

void AHexReplicatorDebugDynamicActor::SetOnlinePropertyPushModel(float value)
{
    if(OnlinePropertyPushModel == value)
        return;

    OnlinePropertyPushModel = value;
    MARK_PROPERTY_DIRTY_FROM_NAME(AHexReplicatorDebugDynamicActor, OnlinePropertyPushModel, this);
}
