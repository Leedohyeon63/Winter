// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WinterGameInstanceSubsystem.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimeUpdatedSignature, float, NewTime);
/**
 * 
 */
UCLASS()
class WINTER_API UWinterGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "TimeSystem")
	void StartTimeSystem(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "TimeSystem")
	void SetCurrentTime(float NewTime);

	UPROPERTY(BlueprintAssignable, Category = "TimeSystem")
	FOnTimeUpdatedSignature OnTimeUpdatedDelegate;

private:
	UFUNCTION()
	void OnTimerTick();

	UPROPERTY(EditAnywhere, Category = "TimeSystem", meta = (AllowPrivateAccess = "true"))
	float CurrentTime = 12.0f;

	UPROPERTY(EditAnywhere, Category = "TimeSystem", meta = (AllowPrivateAccess = "true"))
	float TimeSpeedMultiplier = 0.02f;

	FTimerHandle TimeUpdateTimerHandle;

	const float TimerInterval = 0.1f;
};
