#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WeaponProjectilePoolSubsystem.generated.h"

class APawn;
class AWeaponProjectile;

DECLARE_LOG_CATEGORY_EXTERN(LogWeaponProjectilePool, Log, All);

/** [투사체 풀링 추가] 한 투사체 클래스가 생성한 전체 개체와 현재 대기 중인 개체를 보관한다. */
USTRUCT()
struct WINTER_API FWeaponProjectilePoolBucket
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TArray<TObjectPtr<AWeaponProjectile>> AllProjectiles;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AWeaponProjectile>> InactiveProjectiles;

	int32 MaxRetainedSize = 32;
};

/**
 * [투사체 풀링 추가]
 * 현재 월드 안에서 WeaponProjectile을 클래스별로 생성하고 재사용한다.
 * 풀이 부족하면 공격을 취소하지 않고 확장하며, 반환 시 최대 보관 수를 초과한 개체만 제거한다.
 */
UCLASS()
class WINTER_API UWeaponProjectilePoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	/** 지정한 클래스의 풀 크기와 최대 보관 수를 설정하고 필요한 수만큼 미리 생성한다. */
	void ConfigurePool(
		TSubclassOf<AWeaponProjectile> ProjectileClass,
		int32 PrewarmCount,
		int32 MaxRetainedSize);

	/** 대기 중인 투사체를 가져오며 없으면 새로 생성한다. */
	AWeaponProjectile* AcquireProjectile(
		TSubclassOf<AWeaponProjectile> ProjectileClass,
		const FTransform& SpawnTransform,
		AActor* NewOwner,
		APawn* NewInstigator);

	/** 충돌 또는 수명 종료된 투사체를 비활성화해 풀로 돌려보낸다. */
	void ReleaseProjectile(AWeaponProjectile* Projectile);

	UFUNCTION(BlueprintPure, Category = "Weapon|Projectile Pool")
	int32 GetActiveProjectileCount(TSubclassOf<AWeaponProjectile> ProjectileClass) const;

	UFUNCTION(BlueprintPure, Category = "Weapon|Projectile Pool")
	int32 GetInactiveProjectileCount(TSubclassOf<AWeaponProjectile> ProjectileClass) const;

private:
	AWeaponProjectile* CreateProjectile(TSubclassOf<AWeaponProjectile> ProjectileClass);
	void CleanupInvalidProjectiles(FWeaponProjectilePoolBucket& Pool);

	// [투사체 풀링 추가] UPROPERTY로 보관해 비활성 투사체 참조가 GC 중에도 유지되게 한다.
	UPROPERTY(Transient)
	TMap<TSubclassOf<AWeaponProjectile>, FWeaponProjectilePoolBucket> ProjectilePools;
};
