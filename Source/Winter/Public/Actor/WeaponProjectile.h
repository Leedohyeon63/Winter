#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponProjectile.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UPrimitiveComponent;
class UProjectileMovementComponent;
class USphereComponent;
class UWeaponProjectilePoolSubsystem;
class APawn;

/** WeaponManager가 생성하며 충돌한 ASC 대상에게 지정된 GameplayEffect를 한 번 적용한다. */
UCLASS(Blueprintable)
class WINTER_API AWeaponProjectile : public AActor
{
	GENERATED_BODY()

public:
	AWeaponProjectile();

	// [투사체 풀링 추가] 풀 서브시스템이 새 개체를 등록하고 비활성 상태로 초기화할 때 사용한다.
	void AssignToPool(UWeaponProjectilePoolSubsystem* InOwningPool);

	// [투사체 풀링 추가] 대기 중인 개체를 새로운 위치와 소유자로 다시 활성화한다.
	void ActivateFromPool(
		const FTransform& SpawnTransform,
		AActor* NewOwner,
		APawn* NewInstigator);

	// [투사체 풀링 추가] 이동·충돌·공격 정보를 지우고 대기 상태로 전환한다.
	void DeactivateToPool();

	UFUNCTION(BlueprintPure, Category = "Weapon|Projectile Pool")
	bool IsActiveProjectile() const { return bIsActiveProjectile; }

	// [투사체 추가] 생성한 WeaponManager가 공격 출처와 피해 효과를 전달한다.
	void InitializeProjectile(
		AActor* InAttackOwner,
		UAbilitySystemComponent* InSourceAbilitySystem,
		TSubclassOf<UGameplayEffect> InDamageEffect,
		UObject* InSourceObject,
		float InDamageAmount,
		float InSpeed,
		float InLifeSeconds);

protected:
	// [투사체 풀링 추가] AActor의 기본 수명 만료 Destroy 대신 풀 반환을 수행한다.
	virtual void LifeSpanExpired() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	/** 풀에서 활성화될 때 트레일·나이아가라 등을 다시 시작하는 Blueprint 확장 지점이다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Projectile Pool", meta = (DisplayName = "On Activated From Pool"))
	void OnActivatedFromPool();

	/** 풀로 돌아갈 때 트레일·나이아가라 등을 정지·초기화하는 Blueprint 확장 지점이다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Projectile Pool", meta = (DisplayName = "On Deactivated To Pool"))
	void OnDeactivatedToPool();

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
	void ReturnToPool();
	void ResetForPool(bool bNotifyBlueprint);

	UPROPERTY(Transient)
	TObjectPtr<UWeaponProjectilePoolSubsystem> OwningPool;

	UPROPERTY(Transient)
	TObjectPtr<AActor> AttackOwner;

	UPROPERTY(Transient)
	TObjectPtr<UAbilitySystemComponent> SourceAbilitySystem;

	UPROPERTY(Transient)
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(Transient)
	TObjectPtr<UObject> DamageSourceObject;

	// [공통 데미지 처리 추가] 충돌 시 Data.Damage SetByCaller로 전달할 양수 피해량이다.
	float DamageAmount = 0.0f;

	// [투사체 안정성 보완] 같은 프레임의 Overlap/Hit 중복 호출로 피해가 두 번 들어가는 것을 막는다.
	bool bHasImpacted = false;

	// [투사체 풀링 추가] 중복 반환과 비활성 개체의 충돌 처리를 막는 실행 상태다.
	bool bIsActiveProjectile = false;
};
