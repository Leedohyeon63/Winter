#include "Subsystem/LevelTravelSubsystem.h"

#include "Actor/LevelPortal.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "PlayerCharacter.h"

bool ULevelTravelSubsystem::TravelToLevel(
	APlayerCharacter* Player,
	const TSoftObjectPtr<UWorld>& TargetLevel,
	FName TargetPortalId)
{
	if (!IsValid(Player) || TargetLevel.IsNull())
	{
		return false;
	}

	const FString TargetPackageName = TargetLevel.ToSoftObjectPath().GetLongPackageName();
	if (!FPackageName::IsValidLongPackageName(TargetPackageName))
	{
		UE_LOG(LogTemp, Warning, TEXT("[LevelTravel] 유효하지 않은 대상 레벨 경로: %s"), *TargetPackageName);
		return false;
	}

	// [레벨 이동 추가] OpenLevel이 현재 World와 Pawn을 파괴하기 전에 영속 상태를 GameInstance에 복사한다.
	SavedPlayerState = Player->CaptureTravelState();
	if (!SavedPlayerState.bIsValid)
	{
		return false;
	}

	PendingTargetPortalId = TargetPortalId;
	bHasPendingTravel = true;

	UGameplayStatics::OpenLevel(Player, FName(*TargetPackageName), true);
	return true;
}

void ULevelTravelSubsystem::RestorePlayerAfterTravel(APlayerCharacter* Player)
{
	if (!bHasPendingTravel || !IsValid(Player) || !SavedPlayerState.bIsValid)
	{
		return;
	}

	// [레벨 이동 추가] 재진입이나 중복 복원을 막기 위해 먼저 대기 플래그를 내린다.
	bHasPendingTravel = false;

	Player->RestoreTravelState(SavedPlayerState);

	FTransform DestinationTransform;
	if (FindDestinationTransform(Player->GetWorld(), PendingTargetPortalId, DestinationTransform))
	{
		Player->SetActorTransform(DestinationTransform, false, nullptr, ETeleportType::TeleportPhysics);

		if (AController* PlayerController = Player->GetController())
		{
			PlayerController->SetControlRotation(DestinationTransform.Rotator());
		}
	}
	else if (!PendingTargetPortalId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LevelTravel] 대상 포탈 ID를 찾지 못했습니다: %s"), *PendingTargetPortalId.ToString());
	}

	PendingTargetPortalId = NAME_None;
	SavedPlayerState = FPlayerTravelState();
}

bool ULevelTravelSubsystem::FindDestinationTransform(
	UWorld* World,
	FName PortalId,
	FTransform& OutTransform) const
{
	if (!World || PortalId.IsNone())
	{
		return false;
	}

	TArray<AActor*> FoundPortals;
	UGameplayStatics::GetAllActorsOfClass(World, ALevelPortal::StaticClass(), FoundPortals);

	for (AActor* FoundActor : FoundPortals)
	{
		const ALevelPortal* Portal = Cast<ALevelPortal>(FoundActor);
		if (Portal && Portal->GetPortalId() == PortalId)
		{
			OutTransform = Portal->GetArrivalTransform();
			return true;
		}
	}

	return false;
}
