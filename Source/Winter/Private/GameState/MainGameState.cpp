// Fill out your copyright notice in the Description page of Project Settings.


#include "GameState/MainGameState.h"

AMainGameState::AMainGameState()
{
	CurrentWorldState = EWorldState::StateA;
	CurrentWeather = EWeatherState::Clear;
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
