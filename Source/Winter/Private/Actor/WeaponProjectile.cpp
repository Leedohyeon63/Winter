#include "Actor/WeaponProjectile.h"

#include "AbilitySystemComponent.h"
#include "Combat/WinterCombat.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffect.h"
#include "Subsystem/WeaponProjectilePoolSubsystem.h"
#include "TimerManager.h"

AWeaponProjectile::AWeaponProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->InitSphereRadius(8.0f);
	// [투사체 안정성 보완] 공격 정보가 주입될 때까지 충돌을 비활성화한다.
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AWeaponProjectile::HandleOverlap);
	CollisionComponent->OnComponentHit.AddDynamic(this, &AWeaponProjectile::HandleBlockingHit);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = 2000.0f;
	ProjectileMovement->MaxSpeed = 2000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bAutoActivate = false;
}

void AWeaponProjectile::AssignToPool(UWeaponProjectilePoolSubsystem* InOwningPool)
{
	OwningPool = InOwningPool;
	ResetForPool(false);
}

bool AWeaponProjectile::ActivateFromPool(
	const FTransform& SpawnTransform,
	AActor* NewOwner,
	APawn* NewInstigator)
{
	if (!IsValid(OwningPool) || bIsActiveProjectile || SpawnTransform.ContainsNaN())
	{
		ensureMsgf(
			!bIsActiveProjectile,
			TEXT("Projectile pool attempted to acquire an already active projectile: %s"),
			*GetNameSafe(this));
		return false;
	}

	// [투사체 풀링 추가] 이전 사용의 수명 타이머와 충돌 상태를 제거하고 새 발사 위치로 순간 이동한다.
	SetLifeSpan(0.0f);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	bIsActiveProjectile = true;
	bHasImpacted = false;
	SetOwner(NewOwner);
	SetInstigator(NewInstigator);
	SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorHiddenInGame(false);
	SetActorEnableCollision(false);
	SetActorTickEnabled(true);

	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
		ProjectileMovement->SetComponentTickEnabled(false);
	}

	return IsValid(this) && bIsActiveProjectile;
}

bool AWeaponProjectile::DeactivateToPool()
{
	return ResetForPool(true);
}

bool AWeaponProjectile::InitializeProjectile(
	AActor* InAttackOwner,
	UAbilitySystemComponent* InSourceAbilitySystem,
	TSubclassOf<UGameplayEffect> InDamageEffect,
	UObject* InSourceObject,
	float InDamageAmount,
	float InSpeed,
	float InLifeSeconds)
{
	// [하위 호환] 풀을 사용하지 않고 직접 Spawn한 기존 호출도 같은 초기화 경로를 사용할 수 있게 한다.
	if (!bIsActiveProjectile && !IsValid(OwningPool))
	{
		bIsActiveProjectile = true;
		bHasImpacted = false;
		SetActorHiddenInGame(false);
		SetActorEnableCollision(false);
		SetActorTickEnabled(true);
	}

	if (!bIsActiveProjectile
		|| !IsValid(InAttackOwner)
		|| !IsValid(InSourceAbilitySystem)
		|| !InDamageEffect
		|| InDamageAmount <= 0.0f)
	{
		ReturnToPool();
		return false;
	}

	AttackOwner = InAttackOwner;
	SourceAbilitySystem = InSourceAbilitySystem;
	DamageEffect = InDamageEffect;
	DamageSourceObject = InSourceObject;
	DamageAmount = FMath::Max(0.0f, InDamageAmount);
	bHasImpacted = false;

	if (CollisionComponent && InAttackOwner)
	{
		// [투사체 추가] 생성 직후 공격자 자신의 Capsule과 겹쳐 즉시 파괴되지 않게 한다.
		CollisionComponent->IgnoreActorWhenMoving(InAttackOwner, true);
	}

	if (ProjectileMovement)
	{
		const float ResolvedSpeed = FMath::Max(0.0f, InSpeed);
		// [투사체 풀링 추가] 이전 충돌에서 StopSimulating이 UpdatedComponent를 비웠을 수 있으므로 다시 연결한다.
		ProjectileMovement->SetUpdatedComponent(CollisionComponent);
		ProjectileMovement->SetComponentTickEnabled(true);
		ProjectileMovement->InitialSpeed = ResolvedSpeed;
		ProjectileMovement->MaxSpeed = ResolvedSpeed;
		ProjectileMovement->Velocity = GetActorForwardVector() * ResolvedSpeed;
		ProjectileMovement->Activate(true);
	}

	if (CollisionComponent)
	{
		// [투사체 안정성 보완] 출처와 피해 값이 준비된 뒤에만 충돌 판정을 시작한다.
		SetActorEnableCollision(true);
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	SetLifeSpan(FMath::Max(0.1f, InLifeSeconds));
	OnActivatedFromPool();
	return IsValid(this) && bIsActiveProjectile;
}

void AWeaponProjectile::LifeSpanExpired()
{
	// [투사체 풀링 추가] 시간이 끝나도 액터를 Destroy하지 않고 재사용 대기열로 돌려보낸다.
	ReturnToPool();
}

void AWeaponProjectile::HandleOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!bIsActiveProjectile || bHasImpacted)
	{
		return;
	}

	if (TryApplyDamage(OtherActor, bFromSweep ? &SweepResult : nullptr))
	{
		bHasImpacted = true;
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ReturnToPool();
	}
}

void AWeaponProjectile::HandleBlockingHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (!bIsActiveProjectile || bHasImpacted)
	{
		return;
	}

	bHasImpacted = true;
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TryApplyDamage(OtherActor, &Hit);
	ReturnToPool();
}

bool AWeaponProjectile::TryApplyDamage(AActor* TargetActor, const FHitResult* HitResult)
{
	if (!IsValid(TargetActor)
		|| TargetActor == AttackOwner
		|| !IsValid(SourceAbilitySystem)
		|| !DamageEffect
		|| DamageAmount <= 0.0f)
	{
		return false;
	}

	// [공통 데미지 처리 추가] 근접/히트스캔과 동일한 GAS 피해 전달 함수를 사용한다.
	return WinterCombat::ApplyDamageEffect(
		SourceAbilitySystem,
		AttackOwner,
		TargetActor,
		DamageEffect,
		DamageAmount,
		DamageSourceObject,
		HitResult);
}

void AWeaponProjectile::ReturnToPool()
{
	if (!bIsActiveProjectile)
	{
		return;
	}

	bHasImpacted = true;
	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (IsValid(OwningPool))
	{
		OwningPool->ReleaseProjectile(this);
		return;
	}

	// [투사체 풀링 추가] 풀 밖에서 직접 생성된 투사체는 이전 동작과 동일하게 제거한다.
	Destroy();
}

bool AWeaponProjectile::ResetForPool(const bool bNotifyBlueprint)
{
	if (!IsValid(this))
	{
		return false;
	}

	SetLifeSpan(0.0f);
	if (UWorld* World = GetWorld())
	{
		// [투사체 풀링 보완] Blueprint가 액터에 건 타이머도 다음 발사까지 남지 않게 한다.
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	// [투사체 풀링 보완] 콜백 중 재진입하더라도 충돌과 중복 반환이 먼저 차단되게 한다.
	bHasImpacted = true;
	bIsActiveProjectile = false;

	if (CollisionComponent)
	{
		// [투사체 풀링 추가] 이전 공격자 Ignore 설정을 해제해 다음 소유자에게 누적되지 않게 한다.
		if (AttackOwner)
		{
			CollisionComponent->IgnoreActorWhenMoving(AttackOwner, false);
		}
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Velocity = FVector::ZeroVector;
		ProjectileMovement->Deactivate();
		ProjectileMovement->SetComponentTickEnabled(false);
	}

	AttackOwner = nullptr;
	SourceAbilitySystem = nullptr;
	DamageEffect = nullptr;
	DamageSourceObject = nullptr;
	DamageAmount = 0.0f;

	SetOwner(nullptr);
	SetInstigator(nullptr);
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);

	// [투사체 풀링 보완] Blueprint가 여기서 Destroy하더라도 이후 C++가 액터에 접근하지 않는다.
	if (bNotifyBlueprint)
	{
		OnDeactivatedToPool();
	}

	return IsValid(this);
}
