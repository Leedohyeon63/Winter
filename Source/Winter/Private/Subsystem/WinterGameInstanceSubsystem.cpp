// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystem/WinterGameInstanceSubsystem.h"

void UWinterGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UWinterGameInstanceSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimeUpdateTimerHandle);
	}

	Super::Deinitialize();
}

void UWinterGameInstanceSubsystem::StartTimeSystem(UObject* WorldContextObject)
{
	if (UWorld* World = WorldContextObject->GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimeUpdateTimerHandle);

		World->GetTimerManager().SetTimer(TimeUpdateTimerHandle, this, &UWinterGameInstanceSubsystem::OnTimerTick, TimerInterval, true);
	}
}

void UWinterGameInstanceSubsystem::SetCurrentTime(float NewTime)
{
	CurrentTime = FMath::Clamp(NewTime, 0.0f, 24.0f);

	if (OnTimeUpdatedDelegate.IsBound())
	{
		OnTimeUpdatedDelegate.Broadcast(CurrentTime);
	}
}

void UWinterGameInstanceSubsystem::OnTimerTick()
{
	CurrentTime += (TimerInterval * TimeSpeedMultiplier);


	if (CurrentTime >= 24.0f)
	{
		CurrentTime -= 24.0f;
	}

	if (OnTimeUpdatedDelegate.IsBound())
	{
		OnTimeUpdatedDelegate.Broadcast(CurrentTime);
	}
}