// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/WinterGameMode.h"
#include "Subsystem/WinterGameInstanceSubsystem.h" 
#include "Engine/GameInstance.h"

void AWinterGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UWinterGameInstanceSubsystem* WinterSubsystem = GI->GetSubsystem<UWinterGameInstanceSubsystem>())
		{
			WinterSubsystem->StartTimeSystem(this);
		}
	}
}
