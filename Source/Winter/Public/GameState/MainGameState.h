// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "MainGameState.generated.h"

UENUM(BlueprintType)
enum class EWorldState : uint8
{
    StateA UMETA(DisplayName = "State A"),
    StateB UMETA(DisplayName = "State B"),
    StateC UMETA(DisplayName = "State C")
};

UENUM(BlueprintType)
enum class EWeatherState : uint8
{
    Clear UMETA(DisplayName = "Clear"),
    Rain UMETA(DisplayName = "Rain"),
    Snow UMETA(DisplayName = "Snow")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeatherChangedSignature, EWeatherState, NewWeather);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorldStateChangedSignature, EWorldState, NewState);
/**
 * 
 */
UCLASS()
class WINTER_API AMainGameState : public AGameStateBase
{
	GENERATED_BODY()
public:
    AMainGameState();

    // --- 속성 (Properties) ---
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "World Management")
    EWorldState CurrentWorldState = EWorldState::StateA;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "World Management")
    EWeatherState CurrentWeather = EWeatherState::Clear;

    // --- 이벤트 (Events) ---
    UPROPERTY(BlueprintAssignable, Category = "World Management|Events")
    FOnWeatherChangedSignature OnWeatherChanged;

    UPROPERTY(BlueprintAssignable, Category = "World Management|Events")
    FOnWorldStateChangedSignature OnWorldStateChanged;

    // --- 함수 (Functions) ---
    UFUNCTION(BlueprintCallable, Category = "World Management")
    void SetWeatherState(EWeatherState NewWeather);

    UFUNCTION(BlueprintCallable, Category = "World Management")
    void SetWorldState(EWorldState NewState);
};
