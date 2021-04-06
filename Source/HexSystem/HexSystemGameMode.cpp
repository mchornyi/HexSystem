// Copyright Epic Games, Inc. All Rights Reserved.

#include "HexSystemGameMode.h"
#include "HexSystemCharacter.h"
#include "UObject/ConstructorHelpers.h"

AHexSystemGameMode::AHexSystemGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
