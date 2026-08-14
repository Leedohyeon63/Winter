#include "Actor/WeaponProjectile.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffect.h"

AWeaponProjectile::AWeaponProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->InitSphereRadius(8.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
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
}

void AWeaponProjectile::InitializeProjectile(
	AActor* InAttackOwner,
	UAbilitySystemComponent* InSourceAbilitySystem,
	TSubclassOf<UGameplayEffect> InDamageEffect,
	UObject* InSourceObject,
	float InSpeed,
	float InLifeSeconds)
{
	AttackOwner = InAttackOwner;
	SourceAbilitySystem = InSourceAbilitySystem;
	DamageEffect = InDamageEffect;
	DamageSourceObject = InSourceObject;

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
	if (TryApplyDamage(OtherActor, bFromSweep ? &SweepResult : nullptr))
	{
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
	TryApplyDamage(OtherActor, &Hit);
	Destroy();
}

bool AWeaponProjectile::TryApplyDamage(AActor* TargetActor, const FHitResult* HitResult)
{
	if (!IsValid(TargetActor)
		|| TargetActor == AttackOwner
		|| !SourceAbilitySystem
		|| !DamageEffect)
	{
		return false;
	}

	IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(TargetActor);
	UAbilitySystemComponent* TargetAbilitySystem = TargetInterface
		? TargetInterface->GetAbilitySystemComponent()
		: nullptr;
	if (!TargetAbilitySystem)
	{
		return false;
	}

	FGameplayEffectContextHandle EffectContext = SourceAbilitySystem->MakeEffectContext();
	EffectContext.AddInstigator(AttackOwner, AttackOwner);
	EffectContext.AddSourceObject(DamageSourceObject);
	if (HitResult)
	{
		EffectContext.AddHitResult(*HitResult, true);
	}

	const FGameplayEffectSpecHandle SpecHandle = SourceAbilitySystem->MakeOutgoingSpec(
		DamageEffect,
		1.0f,
		EffectContext);
	if (!SpecHandle.IsValid())
	{
		return false;
	}

	TargetAbilitySystem->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	return true;
}
