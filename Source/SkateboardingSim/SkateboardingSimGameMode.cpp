// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkateboardingSimGameMode.h"
#include "SkateboardingSimCharacter.h"
#include "UObject/ConstructorHelpers.h"

ASkateboardingSimGameMode::ASkateboardingSimGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
