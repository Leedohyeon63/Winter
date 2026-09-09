#include "Actor/MonsterSpawnArea.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Monster/BaseMonster.h"
#include "NavigationInvokerComponent.h"
#include "NavigationSystem.h"
#include "Subsystem/MonsterGenSubsystem.h"
#include "Subsystem/MonsterPoolSubsystem.h"

AMonsterSpawnArea::AMonsterSpawnArea()
{
	PrimaryActorTick.bCanEverTick = false;
	SpawnBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnBounds"));
	SetRootComponent(SpawnBounds);
	SpawnBounds->SetBoxExtent(FVector(1500.0f, 1500.0f, 500.0f));
	SpawnBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnBounds->SetGenerateOverlapEvents(false);
	SpawnBounds->SetCanEverAffectNavigation(false);
	SpawnBounds->ShapeColor = FColor::Green;
	NavigationInvoker = CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavigationInvoker"));
	NavigationInvoker->SetAutoActivate(false);
}

void AMonsterSpawnArea::BeginPlay()
{
	Super::BeginPlay();
	if (UMonsterGenSubsystem* Spawner = GetWorld()->GetSubsystem<UMonsterGenSubsystem>())
	{
		Spawner->RegisterSpawnArea(this);
	}
}

void AMonsterSpawnArea::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UMonsterGenSubsystem* Spawner = GetWorld()->GetSubsystem<UMonsterGenSubsystem>())
	{
		Spawner->UnregisterSpawnArea(this);
	}
	if (UMonsterPoolSubsystem* Pool = GetWorld()->GetSubsystem<UMonsterPoolSubsystem>())
	{
		ReleaseAllMonsters(*Pool);
	}
	UpdateNavigation(false);
	Super::EndPlay(EndPlayReason);
}

bool AMonsterSpawnArea::IntersectsPlayerRange(const FVector& PlayerLocation, float Radius) const
{
	if (!SpawnBounds || Radius <= 0.0f)
	{
		return false;
	}
	const FTransform& Transform = SpawnBounds->GetComponentTransform();
	const FVector LocalPlayer = Transform.InverseTransformPosition(PlayerLocation);
	const FVector Extent = SpawnBounds->GetUnscaledBoxExtent();
	const FVector ClosestLocal = LocalPlayer.BoundToBox(-Extent, Extent);
	const FVector ClosestWorld = Transform.TransformPosition(ClosestLocal);
	return FVector::DistSquared(PlayerLocation, ClosestWorld) <= FMath::Square(Radius);
}

bool AMonsterSpawnArea::IsSpawnLocationAllowed(const FVector& Location, const FVector& PlayerLocation, float Radius) const
{
	if (!SpawnBounds || Radius <= 0.0f || Location.ContainsNaN())
	{
		return false;
	}
	const FVector LocalPoint = SpawnBounds->GetComponentTransform().InverseTransformPosition(Location);
	const FVector Extent = SpawnBounds->GetUnscaledBoxExtent();
	return FMath::Abs(LocalPoint.X) <= Extent.X && FMath::Abs(LocalPoint.Y) <= Extent.Y
		&& FMath::Abs(LocalPoint.Z) <= Extent.Z
		&& FVector::DistSquared(Location, PlayerLocation) <= FMath::Square(Radius);
}

bool AMonsterSpawnArea::IsOwnedInstanceActive(const FAreaMonsterInstance& Instance) const
{
	const ABaseMonster* Monster = Instance.Monster.Get();
	return IsValid(Monster) && Monster->IsActiveMonster()
		&& Monster->GetLifeGeneration() == Instance.LifeGeneration;
}

int32 AMonsterSpawnArea::GetActiveMonsterCount() const
{
	int32 Count = 0;
	for (const FAreaMonsterInstance& Instance : SpawnedMonsters)
	{
		Count += IsOwnedInstanceActive(Instance) ? 1 : 0;
	}
	return Count;
}

void AMonsterSpawnArea::RefreshMonsters(const FVector& PlayerLocation, float DespawnRadius, UMonsterPoolSubsystem& Pool)
{
	const TArray<FAreaMonsterInstance> Instances = SpawnedMonsters;
	for (const FAreaMonsterInstance& Instance : Instances)
	{
		if (!IsValid(this) || IsActorBeingDestroyed())
		{
			return;
		}
		ABaseMonster* Monster = Instance.Monster.Get();
		const bool bOwned = IsOwnedInstanceActive(Instance);
		if (!bOwned || !bSpawnEnabled || Monster->GetClass() != MonsterClass.Get()
			|| FVector::DistSquared(PlayerLocation, Monster->GetActorLocation()) > FMath::Square(DespawnRadius))
		{
			SpawnedMonsters.RemoveAll([&Instance](const FAreaMonsterInstance& Entry)
			{
				return Entry.Monster == Instance.Monster && Entry.LifeGeneration == Instance.LifeGeneration;
			});
			if (bOwned)
			{
				Pool.ReleaseMonster(Monster);
			}
		}
	}
}

void AMonsterSpawnArea::ReleaseAllMonsters(UMonsterPoolSubsystem& Pool)
{
	TArray<FAreaMonsterInstance> Instances = MoveTemp(SpawnedMonsters);
	SpawnedMonsters.Reset();
	for (const FAreaMonsterInstance& Instance : Instances)
	{
		if (IsOwnedInstanceActive(Instance))
		{
			Pool.ReleaseMonster(Instance.Monster.Get());
		}
	}
}

void AMonsterSpawnArea::UpdateNavigation(bool bNeeded)
{
	if (bNeeded)
	{
		const float GenerationRadius = SpawnBounds->GetScaledBoxExtent().Size() + 1000.0f;
		NavigationInvoker->SetGenerationRadii(GenerationRadius, GenerationRadius + 1000.0f);
		if (!NavigationInvoker->IsActive())
		{
			NavigationInvoker->Activate();
		}
	}
	else if (NavigationInvoker->IsActive())
	{
		NavigationInvoker->Deactivate();
	}
}

bool AMonsterSpawnArea::FindSpawnTransform(const FVector& PlayerLocation, float SpawnRadius, FTransform& OutTransform) const
{
	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	const ABaseMonster* Defaults = MonsterClass ? MonsterClass->GetDefaultObject<ABaseMonster>() : nullptr;
	if (!NavSystem || !Defaults || !SpawnBounds || !IntersectsPlayerRange(PlayerLocation, SpawnRadius))
	{
		return false;
	}
	const UCapsuleComponent* Capsule = Defaults->GetCapsuleComponent();
	// 전체 구역이 아닌 플레이어 범위와 겹치는 AABB를 샘플링해 먼 구역의 가장자리도 처리한다.
	const FBox CandidateBounds = SpawnBounds->Bounds.GetBox().Overlap(
		FBox(PlayerLocation - FVector(SpawnRadius), PlayerLocation + FVector(SpawnRadius)));
	if (!CandidateBounds.IsValid)
	{
		return false;
	}
	for (int32 Attempt = 0; Attempt < FMath::Clamp(SpawnLocationAttempts, 1, 64); ++Attempt)
	{
		const FVector Candidate = FMath::RandPointInBox(CandidateBounds);
		const ANavigationData* NavData = NavSystem->GetNavDataForProps(Defaults->GetNavAgentPropertiesRef(), Candidate);
		FNavLocation NavLocation;
		if (!NavData || !NavSystem->ProjectPointToNavigation(Candidate, NavLocation,
			FVector(100.0f, 100.0f, SpawnBounds->GetScaledBoxExtent().Z + 100.0f), NavData))
		{
			continue;
		}
		const FVector SpawnLocation = NavLocation.Location + FVector(0.0f, 0.0f, Capsule->GetScaledCapsuleHalfHeight() + 2.0f);
		if (!IsSpawnLocationAllowed(NavLocation.Location, PlayerLocation, SpawnRadius)
			|| !IsSpawnLocationAllowed(SpawnLocation, PlayerLocation, SpawnRadius))
		{
			continue;
		}
		const FQuat Rotation = SpawnBounds->GetComponentQuat();
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MonsterAreaSpawn), false, this);
		if (GetWorld()->OverlapBlockingTestByProfile(SpawnLocation, FQuat::Identity,
			Capsule->GetCollisionProfileName(), FCollisionShape::MakeCapsule(
				Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight()), QueryParams))
		{
			continue;
		}
		OutTransform = FTransform(FRotator(0.0f, Rotation.Rotator().Yaw, 0.0f), SpawnLocation);
		return true;
	}
	return false;
}

bool AMonsterSpawnArea::TrySpawnMonster(const FVector& PlayerLocation, float SpawnRadius, UMonsterPoolSubsystem& Pool)
{
	if (!bSpawnEnabled || !MonsterClass || GetActiveMonsterCount() >= FMath::Max(0, MaxMonsters)
		|| GetWorld()->GetTimeSeconds() < NextSpawnTime)
	{
		return false;
	}
	FTransform SpawnTransform;
	if (!FindSpawnTransform(PlayerLocation, SpawnRadius, SpawnTransform))
	{
		return false;
	}
	if (ConfiguredPoolClass != MonsterClass)
	{
		Pool.ConfigurePool(MonsterClass, FMath::Min(MaxMonsters, 4), FMath::Max(MaxMonsters, 16));
		ConfiguredPoolClass = MonsterClass;
	}
	ABaseMonster* Monster = Pool.AcquireMonster(MonsterClass, SpawnTransform);
	if (!IsValid(Monster))
	{
		return false;
	}
	if (!IsValid(this) || IsActorBeingDestroyed() || !bSpawnEnabled || Monster->GetClass() != MonsterClass.Get())
	{
		Pool.ReleaseMonster(Monster);
		return false;
	}
	SpawnedMonsters.Add({Monster, Monster->GetLifeGeneration()});
	NextSpawnTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.1f, SpawnInterval);
	return true;
}
