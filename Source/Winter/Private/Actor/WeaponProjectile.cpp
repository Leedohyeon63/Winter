#include "Actor/WeaponProjectile.h"

#include "Combat/WinterCombat.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffect.h"

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

void AWeaponProjectile::InitializeProjectile(
	AActor* InAttackOwner,
	UAbilitySystemComponent* InSourceAbilitySystem,
	TSubclassOf<UGameplayEffect> InDamageEffect,
	UObject* InSourceObject,
	float InDamageAmount,
	float InSpeed,
	float InLifeSeconds)
{
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
		ProjectileMovement->InitialSpeed = ResolvedSpeed;
		ProjectileMovement->MaxSpeed = ResolvedSpeed;
		ProjectileMovement->Velocity = GetActorForwardVector() * ResolvedSpeed;
		ProjectileMovement->Activate(true);
	}

	if (CollisionComponent)
	{
		// [투사체 안정성 보완] 출처와 피해 값이 준비된 뒤에만 충돌 판정을 시작한다.
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	SetLifeSpan(FMath::Max(0.1f, InLifeSeconds));
}

void AWeaponProjectile::HandleOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (bHasImpacted)
	{
		return;
	}

	if (TryApplyDamage(OtherActor, bFromSweep ? &SweepResult : nullptr))
	{
		bHasImpacted = true;
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Destroy();
	}
}

void AWeaponProjectile::HandleBlockingHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (bHasImpacted)
	{
		return;
	}

	bHasImpacted = true;
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TryApplyDamage(OtherActor, &Hit);
	Destroy();
}

bool AWeaponProjectile::TryApplyDamage(AActor* TargetActor, const FHitResult* HitResult)
{
	if (!IsValid(TargetActor)
		|| TargetActor == AttackOwner
		|| !SourceAbilitySystem
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
