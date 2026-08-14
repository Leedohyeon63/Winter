#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "InputActionValue.h"
#include "GameplayEffectTypes.h"
#include "Struct/PlayerTravelState.h"
#include "UI/PlayerViewModel.h"
#include "PlayerCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class UPlayerInventoryComponent;
class UWeaponManagerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatChanged, float, CurrentValue, float, MaxValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHoverInteractableChanged, bool, bIsInteractable, FString, PromptText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryToggleRequestedSignature);

UCLASS()
class WINTER_API APlayerCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APlayerCharacter();

	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	UPlayerInventoryComponent* GetInventoryComponent() const
	{
		return InventoryComponent;
	}

	// [웨폰 매니저 추가] UI, AnimNotify, Blueprint가 현재 플레이어의 무기 시스템에 접근할 때 사용한다.
	UFUNCTION(BlueprintPure, Category = "Weapon")
	UWeaponManagerComponent* GetWeaponManagerComponent() const
	{
		return WeaponManagerComponent;
	}

	// [레벨 이동 추가] 레벨 전환 서브시스템이 Pawn 재생성 전후의 플레이어 상태를 전달한다.
	FPlayerTravelState CaptureTravelState() const;
	void RestoreTravelState(const FPlayerTravelState& InState);

	UPROPERTY(BlueprintAssignable, Category = "UI|Stats")
	FOnStatChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI|Stats")
	FOnStatChanged OnStaminaChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI|Stats")
	FOnStatChanged OnMentalityChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI|Interaction")
	FOnHoverInteractableChanged OnHoverInteractableChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI|Inventory")
	FOnInventoryToggleRequestedSignature OnInventoryToggleRequested;
protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	class UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	class UPlayerStatAttributeSet* AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	UPlayerInventoryComponent* InventoryComponent;

	// [웨폰 매니저 추가] 장착 무기 선택과 공격 판정을 담당한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	UWeaponManagerComponent* WeaponManagerComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player|Stamina")
	float StaminaRegenDelay = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities|Effects")
	TSubclassOf<class UGameplayEffect> StaminaDrainEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities|Effects")
	TSubclassOf<class UGameplayEffect> StaminaRegenEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractRange = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* InventoryAction;

	// [웨폰 매니저 추가] 현재 활성 무기의 공격을 시작하는 Enhanced Input Action이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Weapon")
	UInputAction* WeaponAttackAction;

	// [웨폰 매니저 추가] 주무기와 보조무기를 전환하는 Enhanced Input Action이다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Weapon")
	UInputAction* SwitchWeaponAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	int32 DefaultMappingPriority = 0;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Interaction")
	class UInteractableComponent* CurrentHoveredComponent;

	FActiveGameplayEffectHandle ActiveStaminaDrainHandle;

	FActiveGameplayEffectHandle ActiveStaminaRegenHandle;

	FTimerHandle StaminaRegenTimerHandle;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	UFUNCTION(BlueprintCallable, Category = "UI|Stats")
	void OnSprintInput(bool bIsSprinting);

	void StartStaminaRegen();

	void TryInteract();

	void RequestInventoryToggle();

	void TryWeaponAttack();

	void SwitchActiveWeapon();

	// [레벨 이동 추가] 새 레벨에서 Controller와 포탈 액터가 준비된 다음 틱에 복원을 요청한다.
	void RestoreAfterLevelTravel();

	// [멘탈리티 월드 상태 추가] 현재 GAS 값을 MainGameState 중앙 계층에 전달한다.
	void UpdateMentalityWorldState(bool bForceBroadcast);

	void CheckCrosshairHover();

	void HealthChangedCallback(const struct FOnAttributeChangeData& Data);
	void StaminaChangedCallback(const struct FOnAttributeChangeData& Data);
	void MentalityChangedCallback(const struct FOnAttributeChangeData& Data);


};
