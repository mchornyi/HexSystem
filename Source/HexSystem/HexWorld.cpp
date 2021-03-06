// Fill out your copyright notice in the Description page of Project Settings.

#include "HexWorld.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "Online/DebugActors/HexReplicatorDebugActor.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/AssetManager.h"
#include "CommonCVars.h"

DECLARE_LOG_CATEGORY_EXTERN(HexTrace, Log, All);

DEFINE_LOG_CATEGORY(HexTrace);

using namespace hexsystem;

/**
    @name: SpawnBlueprintFromPath<T>(UWorld* MyWorld, const FString
   PathToBlueprint, const FVector SpawnLocation, FRotator SpawnRotation,
   FActorSpawnParameters SpawnInfo)
    @param: Pointer to loaded world.
    @param: Path to the Blueprint we want to spawn.
    @param: Spawn Loaction for the new Blueprint actor.
    @param: Spawn Rotation for the new Blueprint actor.
    @param: FActorSpawnParameters for the new Blueprint actor.
    @Description: Spawn a Blueprint from a refrence path in PathToBlueprint.
*/
//template<typename T>
//static FORCEINLINE T*
//SpawnBlueprintFromPath( UWorld* myWorld, const FString pathToBlueprint, const FVector spawnLocation, FRotator spawnRotation, FActorSpawnParameters spawnInfo )
//{
//    if ( !myWorld )
//    {
//        return nullptr;
//    }
//
//    FStringAssetReference itemToReference( pathToBlueprint );
//    UObject* itemObject = itemToReference.ResolveObject( );
//    if ( !itemObject )
//    {
//        return nullptr;
//    }
//
//    UBlueprint* generatedBP = Cast<UBlueprint>( itemObject );
//
//    return myWorld->SpawnActor<T>( generatedBP->GeneratedClass, spawnLocation, spawnRotation, spawnInfo );
//}

#if WITH_EDITOR
namespace
{
    void DrawDebugHex(UWorld* inWorld, const FHexLayout& layout, const FHex& hex, float posZ, bool isActorInside, bool isHexVisible, bool isHexCovered);
    void DrawCharacterInfo(UWorld* inWorld, float actorHexWorldPosZ);

    static TAutoConsoleVariable<int> CVar_HexDistVisibility(TEXT("hex.HexDistVisibility"), 3,TEXT("Defines the distance of hex visibility in a context of hex model."),         ECVF_Default);

    static TAutoConsoleVariable<float> CVar_LineThickness(TEXT("hex.LineThickness"), 10.0f, TEXT("Defines the hex line thickness for debug visualizing."), ECVF_Default);

    static TAutoConsoleVariable<int> CVar_DebugActorNum(TEXT("hex.Debug.ActorNumInHex"), 1, TEXT("Defines the number per hex of actor for replication graph performance test."),         ECVF_Default);

    static TAutoConsoleVariable<int> CVar_DebugCharacterCullDist(TEXT("hex.Debug.CharacterCullDist"), 100, TEXT("Defines the radius of hex coverage."), ECVF_Default);

    const float GDebugDrawLifeTimeSec = 1.0f;
} // namespace
#endif

// Sets default values
AHexWorld::AHexWorld()
    : HexLayout(LayoutPointy, FSize(100, 100), FOrigin(0, 0))
      , HexRingsNum(6)
      , HexDistVisibility(3)
      , bHexGenerateTrigger(false)
{
    // Set this actor to call Tick() every frame.  You can turn this off to
    // improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = true;

#if WITH_EDITOR
    //CVar_WorldHexSize.AsVariable( )->SetOnChangedCallback(
    //    FConsoleVariableDelegate::CreateLambda( [ this ]( IConsoleVariable* Variable )
    //{
    //    UE_LOG( HexTrace, Display, TEXT( __FUNCTION__ ) );
    //    const float size= FMath::Clamp<float>( CVar_WorldHexSize.GetValueOnGameThread( ), 1, 10000 );
    //    HexLayout.size = { size, size };
    //} ) );

    //CVar_WorldHexRingsNum.AsVariable( )->SetOnChangedCallback(
    //    FConsoleVariableDelegate::CreateLambda( [ this ]( IConsoleVariable* Variable )
    //{
    //    UE_LOG( HexTrace, Display, TEXT( __FUNCTION__ ) );
    //    HexRingsNum = FMath::Clamp<uint32>( CVar_WorldHexRingsNum.GetValueOnGameThread( ), 1, 100 );
    //} ) );

    CVar_HexDistVisibility.AsVariable()->SetOnChangedCallback(
        FConsoleVariableDelegate::CreateLambda([ this ](IConsoleVariable* Variable)
        {
            UE_LOG(HexTrace, Display, TEXT( __FUNCTION__ ));
            HexDistVisibility = FMath::Clamp<uint32>(CVar_HexDistVisibility.GetValueOnGameThread(), 1, 100);;
        }));
#endif
}

// Called when the game starts or when spawned
void AHexWorld::BeginPlay()
{
    Super::BeginPlay();

    // Call RepeatingFunction once per second, starting two seconds from now.
#if WITH_EDITOR
    if(!mDebugDrawTimerHandler.IsValid())
        GetWorldTimerManager().SetTimer(mDebugDrawTimerHandler, this, &AHexWorld::TickInEditor, GDebugDrawLifeTimeSec, true);
#endif
}

#if WITH_EDITORONLY_DATA
void AHexWorld::PostEditChangeProperty(FPropertyChangedEvent& e)
{
    UE_LOG(HexTrace, Display, TEXT( __FUNCTION__ ));

    const FName propertyName = (e.Property != NULL) ? e.Property->GetFName() : NAME_None;
    if(propertyName == GET_MEMBER_NAME_CHECKED(AHexWorld, bHexGenerateTrigger))
    {
        if(bHexGenerateTrigger == true)
        {
            Generate();
            bHexGenerateTrigger = false;
        }
    }

    Super::PostEditChangeProperty(e);

    GWorldHexRingsNum = HexRingsNum;
    GWorldHexSize = HexLayout.size.X;
}
#endif

////for TArrays:
// void ACustomClass::PostEditChangeChainProperty( struct
// FPropertyChangedChainEvent& e )
//{
//    int32 index = e.GetArrayIndex( TEXT( "Meshes" ) ); //checks skipped
//    UStaticMesh* mesh = Meshes[ index ]; //changed mesh
//    Super::PostEditChangeChainProperty( e );
//}

// Called every frame
void AHexWorld::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

#if WITH_EDITOR
    if(!mDebugDrawTimerHandler.IsValid())
        GetWorldTimerManager().SetTimer(mDebugDrawTimerHandler, this, &AHexWorld::TickInEditor, GDebugDrawLifeTimeSec, true);
#endif
}

void AHexWorld::PostLoad()
{
    Super::PostLoad();

    GWorldHexRingsNum = HexRingsNum;
    GWorldHexSize = HexLayout.size.X;

    //TODO: Doesn't work for Blueprints!
    //TArray<AActor*> foundActors;
    //UGameplayStatics::GetAllActorsOfClass( GetWorld( ), AHexReplicatorDebugActor::StaticClass( ), foundActors );
    //Global_RepActorsNum = foundActors.Num();
}

//////////////////////////////////////////////////////////////////////////
/// WITH_EDITOR ///////////////////////////////////////////////////
#if WITH_EDITOR

void AHexWorld::TickInEditor()
{
    const FVector& actorWorlLoc = GetTransform().GetLocation();
    HexLayout.origin = {actorWorlLoc.X, actorWorlLoc.Y};

    FHex currentHex;
    TArray<FHex> hexesCovered;

    if(APlayerController* playerController = UGameplayStatics::GetPlayerController(GetWorld(), 0))
    {
        FVector viewLocation;
        FRotator viewRotation = playerController->GetControlRotation();
        playerController->GetPlayerViewPoint(viewLocation, viewRotation);

        const FVector2D characterLoc(viewLocation);
        currentHex = FHexModel::HexRound(FHexModel::PixelToHex(HexLayout, characterLoc));
        hexesCovered = FHexModel::HexCoverage(HexLayout, characterLoc, CVar_DebugCharacterCullDist.GetValueOnGameThread(), HexRingsNum);
    }

    for(const auto& hex : HexMap)
    {
        const bool isActorInside = (hex == currentHex);
        const bool isHexVisible =
            FHexModel::HexDistance(hex, currentHex) <= HexDistVisibility;

        const bool isHexCovered = hexesCovered.Contains(hex);

        DrawDebugHex(GetWorld(), HexLayout, hex, actorWorlLoc.Z, isActorInside, isHexVisible, isHexCovered);
    }

    DrawCharacterInfo(GetWorld(), GetActorLocation().Z);
}

void AHexWorld::Generate()
{
    UE_LOG(HexTrace, Display, TEXT( __FUNCTION__ ));

    if(HexRingsNum == HexMap.Num())
    {
        return;
    }

    FHexModel::GenerateHexMap(HexMap, HexRingsNum);
}

bool AHexWorld::ShouldTickIfViewportsOnly() const
{
    return true;
}

void AHexWorld::DebugGenerateRepActors()
{
    UE_LOG(HexTrace, Display, TEXT( __FUNCTION__ ));

    for(TObjectIterator<AHexReplicatorDebugActor> itr; itr; ++itr)
        (*itr)->Destroy();

    for(const auto& hex : HexMap)
    {
        FVector spawnLocation(hexsystem::FHexModel::HexToPixel(HexLayout, hex), 0.0f);
        FRotator spawnRotation(0.0f, 0.0f, 0.0f);

        FActorSpawnParameters spawnParameters;
        spawnParameters.Instigator = GetInstigator();
        spawnParameters.Owner = this;

        auto spawnRawActor = [ & ]()
        {
            const auto debugActorNum = FMath::Clamp(CVar_DebugActorNum.GetValueOnGameThread(), 0, 1000);
            for(int i = 0; i < debugActorNum; ++i)
            {
                auto* spawnedActor = GetWorld()->SpawnActor<AHexReplicatorDebugActor>(spawnLocation, spawnRotation, spawnParameters);
                spawnedActor->GetRootComponent()->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
                spawnedActor->SetFolderPath(TEXT("/Debug/HexReplicatorDebugActor"));
            }
        };

        auto spawnBlueprintActor = [ & ]()
        {
            static auto SItemToStream = TSoftClassPtr<AHexReplicatorDebugActor>(FSoftObjectPath(TEXT("/Game/ThirdPersonCPP/Blueprints/BP_HexReplicatorDebugActor.BP_HexReplicatorDebugActor_C")));
            static auto* SStreamedActor = SItemToStream.LoadSynchronous();

            if(SStreamedActor)
            {
                auto debugActorNum = FMath::Clamp(CVar_DebugActorNum.GetValueOnGameThread(), 0, 1000);
                for(int i = 0; i < debugActorNum; ++i)
                {
                    auto* spawnedActor = GetWorld()->SpawnActor<AHexReplicatorDebugActor>(SStreamedActor, spawnLocation, spawnRotation, spawnParameters);
                    spawnedActor->SetFolderPath(TEXT("/Debug/HexReplicatorDebugActor"));
                }
            }
        };

        //spawnBlueprintActor( );
        spawnRawActor();

        /*const FSoftObjectPath itemToStream( TEXT("Blueprint'/Game/ThirdPersonCPP/Blueprints/BP_HexReplicatorDebugActor.BP_HexReplicatorDebugActor'") );
        FStreamableManager& streamable = UAssetManager::Get( ).GetStreamableManager( );
        const auto req = streamable.RequestSyncLoad( itemToStream );
        const auto* spawnedActor = Cast<UBlueprint>(req->GetLoadedAsset( ));
        GetWorld( )->SpawnActor<AHexReplicatorDebugActor>( spawnedActor->GeneratedClass, spawnLocation, spawnRotation, spawnParameters );*/
    }
}

namespace
{
    void DrawDebugHex(UWorld* inWorld, const FHexLayout& layout, const FHex& hex, float posZ, bool isActorInside, bool isHexVisible, bool isHexCovered)
    {
        check(inWorld);

        check(isHexVisible || !isActorInside);

        const TArray<FPoint> corners = FHexModel::PolygonCorners(layout, hex);

        posZ += isActorInside ? 3.0f : 0.0f;

        FColor Color = isActorInside ? FColor(0, 255, 0) : FColor(255, 0, 0);
        if(!isHexVisible)
        {
            Color = FColor(155, 158, 163);
            posZ += 3.0f;
        }

        if(isHexCovered)
        {
            Color = FColor(235, 235, 52);
            posZ += 3.0f;
        }

        static bool bPersistentLines = false;
        static uint8 DepthPriority = 0;
        const float Thickness = CVar_LineThickness.GetValueOnGameThread();

        FVector LineStart(0.0f, 0.0f, posZ);
        FVector LineEnd(0.0f, 0.0f, posZ);

        for(int i = 0; i < corners.Num() - 1; ++i)
        {
            const FPoint& coner = corners[i];
            const FPoint& conerNext = corners[i + 1];

            LineStart.X = coner.X;
            LineStart.Y = coner.Y;

            LineEnd.X = conerNext.X;
            LineEnd.Y = conerNext.Y;

            DrawDebugLine(inWorld, LineStart, LineEnd, Color, bPersistentLines, GDebugDrawLifeTimeSec, DepthPriority, Thickness);
        }

        const FPoint& coner = corners[corners.Num() - 1];
        const FPoint& conerNext = corners[0];

        LineStart.X = coner.X;
        LineStart.Y = coner.Y;

        LineEnd.X = conerNext.X;
        LineEnd.Y = conerNext.Y;

        DrawDebugLine(inWorld, LineStart, LineEnd, Color, bPersistentLines, GDebugDrawLifeTimeSec, DepthPriority, Thickness);
    }

    void DrawCharacterInfo(UWorld* inWorld, float actorHexWorldPosZ)
    {
        check(inWorld);

        APlayerController* playerController = UGameplayStatics::GetPlayerController(inWorld, 0);
        if(!playerController)
            return;

        FVector viewLocation;
        FRotator viewRotation = playerController->GetControlRotation();
        playerController->GetPlayerViewPoint(viewLocation, viewRotation);

        viewLocation.Z = actorHexWorldPosZ;

        DrawDebugPoint(inWorld, viewLocation, 10.0f, FColor(155, 0, 0), false, -1.0f, 100);

        DrawCircle(inWorld, viewLocation,
                   {1, 0, 0}, {0, 1, 0},
                   FColor::White, CVar_DebugCharacterCullDist.GetValueOnGameThread(), 24,
                   false, -1, SDPG_World, 2);
    }
} // namespace

#endif
//////////////////////////////////////////////////////////////////////////
