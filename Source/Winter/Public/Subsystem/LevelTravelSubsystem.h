#pragma once

#include "CoreMinimal.h"
#include "Struct/PlayerTravelState.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LevelTravelSubsystem.generated.h"

class APlayerCharacter;
class UWorld;

UCLASS()
class WINTER_API ULevelTravelSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	bool TravelToLevel(
		APlayerCharacter* Player,
		const TSoftObjectPtr<UWorld>& TargetLevel,
		FName TargetPortalId);

	void RestorePlayerAfterTravel(APlayerCharacter* Player);

private:
	bool FindDestinationTransform(UWorld* World, FName PortalId, FTransform& OutTransform) const;

	UPROPERTY()
	FPlayerTravelState SavedPlayerState;

	UPROPERTY()
	FName PendingTargetPortalId = NAME_None;

	bool bHasPendingTravel = false;
};
