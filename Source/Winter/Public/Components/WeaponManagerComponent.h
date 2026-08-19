#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DataAsset/ItemDefinitionDataAsset.h"
#include "WeaponManagerComponent.generated.h"

class UAbilitySystemComponent;
class UAnimMontage;
class UPlayerInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnActiveWeaponChangedSignature,
	EEquipmentSlot,
	ActiveSlot,
	UItemDefinitionDataAsset*,
	ActiveWeapon);

/**
 * 플레이어의 주무기/보조무기를 선택하고 무기의 AttackTypeTag에 맞는 판정을 실행한다.
 * 아이템 보관과 장착은 기존 PlayerInventoryComponent에 맡기고 이 컴포넌트는 전투만 담당한다.
 */
UCLASS(ClassGroup = (Combat), meta = (BlueprintSpawnableComponent))
class WINTER_API UWeaponManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponManagerComponent();

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Events")
	FOnActiveWeaponChangedSignature OnActiveWeaponChanged;

	// [웨폰 매니저 추가] 현재 활성 무기로 공격을 시작하고 쿨다운 또는 잘못된 설정이면 false를 반환한다.
	UFUNCTION(BlueprintCallable, Category = "Weapon|Attack")
	bool StartAttack();

	// [웨폰 매니저 추가] AnimNotify에서 호출해 대기 중인 근접/즉발/투사체 판정을 한 번만 실행한다.
	UFUNCTION(BlueprintCallable, Category = "Weapon|Attack")
	bool ExecutePendingAttack();

	// [공격 대기 안정성 보완] 몽타주 중단·사망·무기 전환 시 남은 공격 판정을 명시적으로 취소한다.
	UFUNCTION(BlueprintCallable, Category = "Weapon|Attack")
	void CancelPendingAttack();

	UFUNCTION(BlueprintCallable, Category = "Weapon|Equipment")
	bool SwitchWeapon();

	UFUNCTION(BlueprintCallable, Category = "Weapon|Equipment")
	bool SetActiveWeaponSlot(EEquipmentSlot NewSlot);

	UFUNCTION(BlueprintPure, Category = "Weapon|Equipment")
	EEquipmentSlot GetActiveWeaponSlot() const { return ActiveWeaponSlot; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Equipment")
	UItemDefinitionDataAsset* GetActiveWeapon() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// [웨폰 매니저 추가] 개발 중 판정 범위를 월드에 표시할지 지정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Debug")
	bool bDrawDebugTraces = false;

private:
	UFUNCTION()
	void HandleInventoryChanged();

	void HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void RefreshActiveWeapon();
	bool ExecuteMeleeAttack(UItemDefinitionDataAsset* WeaponDefinition);
	bool ExecuteHitscanAttack(UItemDefinitionDataAsset* WeaponDefinition);
	bool ExecuteProjectileAttack(UItemDefinitionDataAsset* WeaponDefinition);
	bool ApplyDamageEffect(
		AActor* TargetActor,
		UItemDefinitionDataAsset* WeaponDefinition,
		const FHitResult* HitResult = nullptr) const;

	UPROPERTY(Transient)
	TObjectPtr<UPlayerInventoryComponent> InventoryComponent;

	UPROPERTY(VisibleInstanceOnly, Category = "Weapon|Equipment")
	EEquipmentSlot ActiveWeaponSlot = EEquipmentSlot::PrimaryWeapon;

	UPROPERTY(Transient)
	TObjectPtr<UItemDefinitionDataAsset> LastActiveWeapon;

	UPROPERTY(Transient)
	TObjectPtr<UItemDefinitionDataAsset> PendingAttackWeapon;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> PendingAttackMontage;

	float NextAttackAllowedTime = 0.0f;
};
