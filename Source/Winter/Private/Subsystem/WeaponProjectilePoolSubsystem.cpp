#include "Subsystem/WeaponProjectilePoolSubsystem.h"

#include "Actor/WeaponProjectile.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

// [투사체 풀링 추가] 생성·재사용 여부를 Output Log에서 따로 필터링할 수 있게 한다.
DEFINE_LOG_CATEGORY(LogWeaponProjectilePool);

void UWeaponProjectilePoolSubsystem::Deinitialize()
{
	// [투사체 풀링 추가] 월드 종료 중 투사체가 이미 사라져도 풀 참조를 안전하게 정리한다.
	for (TPair<TSubclassOf<AWeaponProjectile>, FWeaponProjectilePoolBucket>& Pair : ProjectilePools)
	{
		for (AWeaponProjectile* Projectile : Pair.Value.AllProjectiles)
		{
			if (IsValid(Projectile))
			{
				Projectile->AssignToPool(nullptr);
			}
		}
	}

	ProjectilePools.Empty();
	Super::Deinitialize();
}

void UWeaponProjectilePoolSubsystem::ConfigurePool(
	const TSubclassOf<AWeaponProjectile> ProjectileClass,
	const int32 PrewarmCount,
	const int32 MaxRetainedSize)
{
	if (!ProjectileClass || !GetWorld())
	{
		return;
	}

	FWeaponProjectilePoolBucket& Pool = ProjectilePools.FindOrAdd(ProjectileClass);
	CleanupInvalidProjectiles(Pool);

	Pool.MaxRetainedSize = FMath::Max(1, MaxRetainedSize);
	const int32 SafePrewarmCount = FMath::Clamp(PrewarmCount, 0, Pool.MaxRetainedSize);

	while (Pool.AllProjectiles.Num() < SafePrewarmCount)
	{
		AWeaponProjectile* Projectile = CreateProjectile(ProjectileClass);
		if (!Projectile)
		{
			break;
		}

		Pool.AllProjectiles.Add(Projectile);
		Pool.InactiveProjectiles.Add(Projectile);
	}

	// [투사체 풀링 추가] 설정을 낮췄다면 사용 중인 개체는 건드리지 않고 대기 개체부터 정리한다.
	while (Pool.AllProjectiles.Num() > Pool.MaxRetainedSize
		&& !Pool.InactiveProjectiles.IsEmpty())
	{
		AWeaponProjectile* ExcessProjectile = Pool.InactiveProjectiles.Pop(EAllowShrinking::No);
		Pool.AllProjectiles.RemoveSingleSwap(ExcessProjectile);
		if (IsValid(ExcessProjectile))
		{
			ExcessProjectile->AssignToPool(nullptr);
			ExcessProjectile->Destroy();
		}
	}
}

AWeaponProjectile* UWeaponProjectilePoolSubsystem::AcquireProjectile(
	const TSubclassOf<AWeaponProjectile> ProjectileClass,
	const FTransform& SpawnTransform,
	AActor* NewOwner,
	APawn* NewInstigator)
{
	if (!ProjectileClass || !GetWorld())
	{
		return nullptr;
	}

	FWeaponProjectilePoolBucket& Pool = ProjectilePools.FindOrAdd(ProjectileClass);
	CleanupInvalidProjectiles(Pool);

	AWeaponProjectile* Projectile = nullptr;
	while (!Pool.InactiveProjectiles.IsEmpty() && !IsValid(Projectile))
	{
		Projectile = Pool.InactiveProjectiles.Pop(EAllowShrinking::No);
	}
	const bool bReusedProjectile = IsValid(Projectile);

	if (!Projectile)
	{
		// [투사체 풀링 추가] 동시 발사량이 예열 수를 넘으면 공격을 누락하지 않고 풀을 임시 확장한다.
		Projectile = CreateProjectile(ProjectileClass);
		if (!Projectile)
		{
			return nullptr;
		}
		Pool.AllProjectiles.Add(Projectile);
	}

	Projectile->ActivateFromPool(SpawnTransform, NewOwner, NewInstigator);
	UE_LOG(
		LogWeaponProjectilePool,
		Verbose,
		TEXT("Acquire Class=%s Reused=%s Active=%d Inactive=%d"),
		*GetNameSafe(ProjectileClass.Get()),
		bReusedProjectile ? TEXT("true") : TEXT("false"),
		FMath::Max(0, Pool.AllProjectiles.Num() - Pool.InactiveProjectiles.Num()),
		Pool.InactiveProjectiles.Num());
	return Projectile;
}

void UWeaponProjectilePoolSubsystem::ReleaseProjectile(AWeaponProjectile* Projectile)
{
	if (!IsValid(Projectile))
	{
		return;
	}

	const TSubclassOf<AWeaponProjectile> ProjectileClass = Projectile->GetClass();
	FWeaponProjectilePoolBucket* Pool = ProjectilePools.Find(ProjectileClass);
	if (!Pool || !Pool->AllProjectiles.Contains(Projectile))
	{
		// [투사체 풀링 추가] 등록되지 않은 개체는 잘못된 풀 참조를 남기지 않고 기존 방식으로 제거한다.
		Projectile->AssignToPool(nullptr);
		Projectile->Destroy();
		return;
	}

	if (Pool->InactiveProjectiles.Contains(Projectile))
	{
		return;
	}

	Projectile->DeactivateToPool();
	if (!IsValid(Projectile))
	{
		// [투사체 풀링 추가] Blueprint 반환 이벤트에서 제거된 예외적인 개체는 풀 목록에도 남기지 않는다.
		Pool->AllProjectiles.RemoveSingleSwap(Projectile);
		return;
	}

	if (Pool->AllProjectiles.Num() > Pool->MaxRetainedSize)
	{
		// [투사체 풀링 추가] 순간적으로 확장된 풀은 반환 시 최대 보관 수까지만 남긴다.
		Pool->AllProjectiles.RemoveSingleSwap(Projectile);
		UE_LOG(
			LogWeaponProjectilePool,
			Log,
			TEXT("Trimmed Class=%s Retained=%d"),
			*GetNameSafe(ProjectileClass.Get()),
			Pool->AllProjectiles.Num());
		Projectile->AssignToPool(nullptr);
		Projectile->Destroy();
		return;
	}

	Pool->InactiveProjectiles.Add(Projectile);
}

int32 UWeaponProjectilePoolSubsystem::GetActiveProjectileCount(
	const TSubclassOf<AWeaponProjectile> ProjectileClass) const
{
	if (const FWeaponProjectilePoolBucket* Pool = ProjectilePools.Find(ProjectileClass))
	{
		return FMath::Max(0, Pool->AllProjectiles.Num() - Pool->InactiveProjectiles.Num());
	}
	return 0;
}

int32 UWeaponProjectilePoolSubsystem::GetInactiveProjectileCount(
	const TSubclassOf<AWeaponProjectile> ProjectileClass) const
{
	if (const FWeaponProjectilePoolBucket* Pool = ProjectilePools.Find(ProjectileClass))
	{
		return Pool->InactiveProjectiles.Num();
	}
	return 0;
}

AWeaponProjectile* UWeaponProjectilePoolSubsystem::CreateProjectile(
	const TSubclassOf<AWeaponProjectile> ProjectileClass)
{
	UWorld* World = GetWorld();
	if (!World || !ProjectileClass)
	{
		return nullptr;
	}

	const FTransform StorageTransform(
		FRotator::ZeroRotator,
		FVector(0.0f, 0.0f, -100000.0f));

	// [투사체 풀링 추가] BeginPlay 전에 충돌과 표시를 끄기 위해 지연 생성한다.
	AWeaponProjectile* DeferredProjectile = World->SpawnActorDeferred<AWeaponProjectile>(
		ProjectileClass,
		StorageTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!DeferredProjectile)
	{
		return nullptr;
	}

	DeferredProjectile->AssignToPool(this);
	AWeaponProjectile* Projectile = Cast<AWeaponProjectile>(
		UGameplayStatics::FinishSpawningActor(DeferredProjectile, StorageTransform));
	if (Projectile)
	{
		// [투사체 풀링 추가] BeginPlay에서 자동 재생된 Blueprint 이펙트도 정지할 수 있게 반환 이벤트를 호출한다.
		Projectile->DeactivateToPool();
		if (!IsValid(Projectile))
		{
			return nullptr;
		}
		UE_LOG(
			LogWeaponProjectilePool,
			Log,
			TEXT("Created Class=%s Actor=%s"),
			*GetNameSafe(ProjectileClass.Get()),
			*GetNameSafe(Projectile));
	}
	return Projectile;
}

void UWeaponProjectilePoolSubsystem::CleanupInvalidProjectiles(
	FWeaponProjectilePoolBucket& Pool)
{
	Pool.AllProjectiles.RemoveAll([](const TObjectPtr<AWeaponProjectile>& Projectile)
	{
		return !IsValid(Projectile);
	});

	Pool.InactiveProjectiles.RemoveAll([](const TObjectPtr<AWeaponProjectile>& Projectile)
	{
		return !IsValid(Projectile);
	});
}
