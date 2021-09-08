// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelStreamerActor.h"

#include "Components/SphereComponent.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"

//TArray<FString> gLevelsNames;
//TArray<FString> gSubLevelsNames;

namespace
{
	const FString gOverrideNamePostfix("_dynamic_");
	int gServerLevelCounter = 0;
	int gClientSubLevelCounter = 0;
	const float OverlapVolumeSphereRadius = 400.0f;
	const float SubOverlapVolumeSphereRadius = 200.0f;
	int gUUID = 0;

	FString GetShortPackageName(const FString& LongName)
	{
		// Get everything after the last slash
		int32 IndexOfLastDot = INDEX_NONE;
		LongName.FindLastChar('.', IndexOfLastDot);
		return LongName.Mid(IndexOfLastDot + 1);
	}
}

// Sets default values
ALevelStreamerActor::ALevelStreamerActor() : LevelLocation(1000.0f, 0.0f, 0.0f)
                                           , bIsLevelLoaded(false)
                                           , bIsSubLevelLoaded(false)
{
	OverlapVolume = CreateDefaultSubobject<USphereComponent>(TEXT("OverlapVolume"));
	RootComponent = OverlapVolume;

	OverlapVolume->OnComponentBeginOverlap.AddUniqueDynamic(this, &ALevelStreamerActor::OverlapBeginLoad);
	OverlapVolume->SetSphereRadius(OverlapVolumeSphereRadius);

	SubOverlapVolume = CreateDefaultSubobject<USphereComponent>(TEXT("SubOverlapVolume"));
	SubOverlapVolume->OnComponentBeginOverlap.AddUniqueDynamic(this, &ALevelStreamerActor::SubOverlapBeginLoad);
	SubOverlapVolume->OnComponentEndOverlap.AddUniqueDynamic(this, &ALevelStreamerActor::SubOverlapEndUnload);
	SubOverlapVolume->SetSphereRadius(SubOverlapVolumeSphereRadius);

	const FAttachmentTransformRules AttachmentTransformRules(EAttachmentRule::KeepRelative, false);
	SubOverlapVolume->AttachToComponent(GetRootComponent(), AttachmentTransformRules);

	bReplicates = true;
}

void ALevelStreamerActor::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	SetFolderPath(TEXT("/LevelStreamers"));
#endif
}

// Called when the game starts or when spawned
void ALevelStreamerActor::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ALevelStreamerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ALevelStreamerActor::OverlapBeginLoad(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(!HasAuthority())
		return;

	//ACharacter* MyCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);

	//if(OtherActor != Cast<AActor>(MyCharacter))
	//	return;

	if(!Cast<ACharacter>(OtherActor))
		return;

	if(!bIsLevelLoaded)
		LoadLevel();
}

void ALevelStreamerActor::SubOverlapBeginLoad(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(HasAuthority())
		return;

	ACharacter* MyCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);

	if(OtherActor != Cast<AActor>(MyCharacter))
		return;

	if(OverrideLevelName.IsEmpty())
		return;

	if(bIsSubLevelLoaded)
		return;

	LoadSubLevel();
}

void ALevelStreamerActor::SubOverlapEndUnload(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if(HasAuthority())
		return;

	if(!bIsSubLevelLoaded)
		return;

	if(OverrideSubLevelName.IsEmpty())
		return;
	
	FLatentActionInfo LatentInfo;
	LatentInfo.UUID = gUUID;

	UGameplayStatics::UnloadStreamLevel(this, FName(OverrideSubLevelName), LatentInfo, true);

	bIsSubLevelLoaded = false;

	++gUUID;
}

void ALevelStreamerActor::OnLevelLoaded()
{
	NotifyLevelLoaded(OverrideLevelName, LevelLocation);
}

void ALevelStreamerActor::NotifyLevelLoaded_Implementation(const FString& InOverrideName, const FVector& InLocation)
{
	if(HasAuthority())
		return;

	OverrideLevelName = InOverrideName;
	LevelLocation = InLocation;
	
	LoadLevel();
}

void ALevelStreamerActor::LoadLevel()
{
	if(bIsLevelLoaded)
		return;

	// Client must have not empty InOverrideName always
	if(!HasAuthority() && OverrideLevelName.IsEmpty())
		return;

	if(LevelToLoad.IsNone())
		return;

	if(HasAuthority())
	{
		const FString ShortName = GetShortPackageName(LevelToLoad.ToString());

		OverrideLevelName = ShortName + gOverrideNamePostfix + FString::FormatAsNumber(gServerLevelCounter);
		LevelLocation.Y = gServerLevelCounter * 100;
	}

	bool OutSuccess;
	auto* StreamingLevel = ULevelStreamingDynamic::LoadLevelInstance(this, LevelToLoad.ToString(), LevelLocation, FRotator(0, 0, 0), OutSuccess, OverrideLevelName);

	if(HasAuthority())
		StreamingLevel->OnLevelLoaded.AddDynamic(this, &ALevelStreamerActor::OnLevelLoaded);

	bIsLevelLoaded = true;

	++gServerLevelCounter;
}

// Must be loaded after main level only
void ALevelStreamerActor::LoadSubLevel()
{
	if(HasAuthority())
		return;

	if(bIsSubLevelLoaded)
		return;

	if(OverrideLevelName.IsEmpty())
		return;

	if(SubLevelToLoad.IsNone())
		return;

	const FString ShortName = GetShortPackageName(SubLevelToLoad.ToString());
	OverrideSubLevelName = ShortName + gOverrideNamePostfix + FString::FormatAsNumber(gClientSubLevelCounter);

	bool OutSuccess;
	ULevelStreamingDynamic::LoadLevelInstance(this, SubLevelToLoad.ToString(), LevelLocation, FRotator(0, 0, 0), OutSuccess, OverrideSubLevelName);

	bIsSubLevelLoaded = true;

	++gClientSubLevelCounter;
}