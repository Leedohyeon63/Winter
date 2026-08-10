#pragma once

#include "CoreMinimal.h"
#include "DataAsset/MentalityWorldConfigDataAsset.h"
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnWeatherChangedSignature,
	EWeatherState,
	NewWeather);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnWorldStateChangedSignature,
	EWorldState,
	NewState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnMentalityWorldStateChangedSignature,
	EMentalityWorldState,
	NewState);


UCLASS()
class WINTER_API AMainGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AMainGameState();

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "World Management")
	EWorldState CurrentWorldState = EWorldState::StateA;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "World Management")
	EWeatherState CurrentWeather = EWeatherState::Clear;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Management|Mentality")
	EMentalityWorldState CurrentMentalityWorldState = EMentalityWorldState::Stable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Management|Mentality")
	float CurrentMentalityPercent = 100.0f;

	UPROPERTY(BlueprintAssignable, Category = "World Management|Events")
	FOnWeatherChangedSignature OnWeatherChanged;

	UPROPERTY(BlueprintAssignable, Category = "World Management|Events")
	FOnWorldStateChangedSignature OnWorldStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "World Management|Mentality")
	FOnMentalityWorldStateChangedSignature OnMentalityWorldStateChanged;

	UFUNCTION(BlueprintCallable, Category = "World Management")
	void SetWeatherState(EWeatherState NewWeather);

	UFUNCTION(BlueprintCallable, Category = "World Management")
	void SetWorldState(EWorldState NewState);

	UFUNCTION(BlueprintCallable, Category = "World Management|Mentality")
	void UpdateMentalityWorldState(
		float CurrentMentality,
		float MaxMentality,
		bool bForceBroadcast = false);

	UFUNCTION(BlueprintPure, Category = "World Management|Mentality")
	UMentalityWorldConfigDataAsset* GetMentalityWorldConfig() const
	{
		return MentalityWorldConfig;
	}

	const FMentalityWorldStateProfile* FindCurrentMentalityProfile() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Management|Mentality")
	TObjectPtr<UMentalityWorldConfigDataAsset> MentalityWorldConfig;

private:
	EMentalityWorldState ResolveFallbackMentalityState(float MentalityPercent) const;
};
