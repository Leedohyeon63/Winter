#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponProjectile.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UPrimitiveComponent;
class UProjectileMovementComponent;
class USphereComponent;

/** WeaponManager가 생성하며 충돌한 ASC 대상에게 지정된 GameplayEffect를 한 번 적용한다. */
UCLASS(Blueprintable)
class WINTER_API AWeaponProjectile : public AActor
{
	GENERATED_BODY()

public:
	AWeaponProjectile();

	// [투사체 추가] 생성한 WeaponManager가 공격 출처와 피해 효과를 전달한다.
	void InitializeProjectile(
		AActor* InAttackOwner,
		UAbilitySystemComponent* InSourceAbilitySystem,
		TSubclassOf<UGameplayEffect> InDamageEffect,
		UObject* InSourceObject,
		float InSpeed,
		float InLifeSeconds);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

private:
	UFUNCTION()
	void HandleOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleBlockingHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		FVector NormalImpulse,
		const FHitResult& Hit);

	bool TryApplyDamage(AActor* TargetActor, const FHitResult* HitResult);

	UPROPERTY(Transient)
	TObjectPtr<AActor> AttackOwner;

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> SourceAbilitySystem;

	UPROPERTY(Transient)
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(Transient)
	TObjectPtr<UObject> DamageSourceObject;
};
