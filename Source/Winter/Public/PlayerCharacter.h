// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "InputActionValue.h"
#include "GameplayEffectTypes.h"
#include "UI/PlayerViewModel.h"
#include "PlayerCharacter.generated.h"

class UInputAction;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStatChanged, float, CurrentValue, float, MaxValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHoverInteractableChanged, bool, bIsInteractable, FString, PromptText);

UCLASS()
class WINTER_API APlayerCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APlayerCharacter();

	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UPROPERTY(BlueprintAssignable, Category = "UI|Stats")
	FOnStatChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI|Stats")
	FOnStatChanged OnStaminaChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI|Stats")
	FOnStatChanged OnMentalityChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI|Interaction")
	FOnHoverInteractableChanged OnHoverInteractableChanged;
protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	class UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	class UPlayerStatAttributeSet* AttributeSet;

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

	void CheckCrosshairHover();

	void HealthChangedCallback(const struct FOnAttributeChangeData& Data);
	void StaminaChangedCallback(const struct FOnAttributeChangeData& Data);
	void MentalityChangedCallback(const struct FOnAttributeChangeData& Data);


};
