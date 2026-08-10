#include "GameState/MainGameState.h"

AMainGameState::AMainGameState()
{
	CurrentWorldState = EWorldState::StateA;
	CurrentWeather = EWeatherState::Clear;
	CurrentMentalityWorldState = EMentalityWorldState::Stable;
	CurrentMentalityPercent = 100.0f;
}

void AMainGameState::SetWeatherState(EWeatherState NewWeather)
{
	if (CurrentWeather != NewWeather)
	{
		CurrentWeather = NewWeather;
		OnWeatherChanged.Broadcast(CurrentWeather);
	}
}

void AMainGameState::SetWorldState(EWorldState NewState)
{
	if (CurrentWorldState != NewState)
	{
		CurrentWorldState = NewState;
		OnWorldStateChanged.Broadcast(CurrentWorldState);
	}
}

void AMainGameState::UpdateMentalityWorldState(
	float CurrentMentality,
	float MaxMentality,
	bool bForceBroadcast)
{
	CurrentMentalityPercent = MaxMentality > KINDA_SMALL_NUMBER
		? FMath::Clamp((CurrentMentality / MaxMentality) * 100.0f, 0.0f, 100.0f)
		: 0.0f;

	const FMentalityWorldStateProfile* ResolvedProfile = MentalityWorldConfig
		? MentalityWorldConfig->FindProfileForPercent(CurrentMentalityPercent)
		: nullptr;

	const EMentalityWorldState NewState = ResolvedProfile
		? ResolvedProfile->State
		: ResolveFallbackMentalityState(CurrentMentalityPercent);

	if (CurrentMentalityWorldState == NewState && !bForceBroadcast)
	{
		return;
	}

	CurrentMentalityWorldState = NewState;

	OnMentalityWorldStateChanged.Broadcast(CurrentMentalityWorldState);
}

const FMentalityWorldStateProfile* AMainGameState::FindCurrentMentalityProfile() const
{
	return MentalityWorldConfig
		? MentalityWorldConfig->FindProfileByState(CurrentMentalityWorldState)
		: nullptr;
}

EMentalityWorldState AMainGameState::ResolveFallbackMentalityState(
	float MentalityPercent) const
{
	if (MentalityPercent >= 75.0f)
	{
		return EMentalityWorldState::Stable;
	}

	if (MentalityPercent >= 50.0f)
	{
		return EMentalityWorldState::Uneasy;
	}

	if (MentalityPercent >= 20.0f)
	{
		return EMentalityWorldState::Distorted;
	}

	return EMentalityWorldState::Critical;
}
