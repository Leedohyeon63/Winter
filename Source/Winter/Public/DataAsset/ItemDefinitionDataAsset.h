// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ItemDefinitionDataAsset.generated.h"

class AWeaponProjectile;
class UAnimMontage;
class UGameplayEffect;
class UTexture2D;

UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	None UMETA(DisplayName = "None"),
	Head UMETA(DisplayName = "Head"),
	Body UMETA(DisplayName = "Body"),
	Hands UMETA(DisplayName = "Hands"),
	Legs UMETA(DisplayName = "Legs"),
	Feet UMETA(DisplayName = "Feet"),
	Backpack UMETA(DisplayName = "Backpack"),
	PrimaryWeapon UMETA(DisplayName = "Primary Weapon"),
	SecondaryWeapon UMETA(DisplayName = "Secondary Weapon")
};

/** 아이템의 공통 정보와 장비 및 무기 설정을 보관한다. */
UCLASS(BlueprintType)
class WINTER_API UItemDefinitionDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FName ItemId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "0.0"))
	float UnitWeight = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 MaxStackSize = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment")
	bool bEquippable = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment", meta = (EditCondition = "bEquippable"))
	EEquipmentSlot EquipmentSlot = EEquipmentSlot::None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Storage", meta = (EditCondition = "bEquippable", ClampMin = "0"))
	int32 GrantedSlots = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Storage", meta = (EditCondition = "bEquippable", ClampMin = "0.0"))
	float GrantedMaxWeight = 0.0f;

	// [웨폰 매니저 추가] 무기가 근접, 즉발, 투사체 중 어떤 판정을 사용할지 Native Gameplay Tag로 지정한다.
	UPROPERTY(
		EditDefaultsOnly,
		BlueprintReadOnly,
		Category = "Weapon",
		meta = (Categories = "Weapon.AttackType"))
	FGameplayTag AttackTypeTag;

	// [웨폰 매니저 추가] 적중한 대상 ASC에 적용할 Instant GameplayEffect다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<UGameplayEffect> AttackDamageEffect;

	// [웨폰 매니저 추가] 공격 시작 시 재생할 몽타주다. 비어 있으면 즉시 공격 판정을 실행한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	// [웨폰 매니저 추가] 활성화하면 몽타주의 AnimNotify가 ExecutePendingAttack을 호출할 때 판정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Animation")
	bool bExecuteAttackOnAnimNotify = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Attack", meta = (ClampMin = "0.0"))
	float AttackRange = 180.0f;

	// [웨폰 매니저 추가] 근접 Sphere Sweep의 반지름이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Attack", meta = (ClampMin = "0.0"))
	float AttackRadius = 55.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Attack", meta = (ClampMin = "0.0"))
	float AttackCooldown = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Projectile")
	TSubclassOf<AWeaponProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Projectile", meta = (ClampMin = "0.0"))
	float ProjectileSpeed = 2000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Projectile", meta = (ClampMin = "0.1"))
	float ProjectileLifeSeconds = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Projectile")
	FName ProjectileSocketName = TEXT("Muzzle");
};
