// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "Attribute/PlayerStatAttributeSet.h" 
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

// Sets default values
APlayerCharacter::APlayerCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UPlayerStatAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* APlayerCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	if (AbilitySystemComponent && AttributeSet)
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute()).AddUObject(this, &APlayerCharacter::HealthChangedCallback);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetStaminaAttribute()).AddUObject(this, &APlayerCharacter::StaminaChangedCallback);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMentalityAttribute()).AddUObject(this, &APlayerCharacter::MentalityChangedCallback);

		OnHealthChanged.Broadcast(AttributeSet->GetHealth(), AttributeSet->GetMaxHealth());
		OnStaminaChanged.Broadcast(AttributeSet->GetStamina(), AttributeSet->GetMaxStamina());
		OnMentalityChanged.Broadcast(AttributeSet->GetMentality(), AttributeSet->GetMaxMentality());
	}
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void APlayerCharacter::OnSprintInput(bool bIsSprinting)
{
	if (bIsSprinting)
	{

		if (AttributeSet && AttributeSet->GetStamina() <= 0.0f)
		{
			return;
		}

		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;

		if (AbilitySystemComponent && StaminaDrainEffect && !ActiveStaminaDrainHandle.IsValid())
		{
			FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
			EffectContext.AddInstigator(this, this);

			FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(StaminaDrainEffect, 1.0f, EffectContext);

			if (SpecHandle.IsValid())
			{
				ActiveStaminaDrainHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

		if (AbilitySystemComponent && ActiveStaminaDrainHandle.IsValid())
		{
			AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveStaminaDrainHandle);
			ActiveStaminaDrainHandle.Invalidate();
		}
	}
}

void APlayerCharacter::StartStaminaRegen()
{
	if (AbilitySystemComponent && StaminaRegenEffect && !ActiveStaminaRegenHandle.IsValid())
	{
		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
		EffectContext.AddInstigator(this, this);
		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(StaminaRegenEffect, 1.0f, EffectContext);

		if (SpecHandle.IsValid())
		{
			ActiveStaminaRegenHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}

void APlayerCharacter::HealthChangedCallback(const FOnAttributeChangeData& Data)
{
	if (AttributeSet)
	{
		OnHealthChanged.Broadcast(Data.NewValue, AttributeSet->GetMaxHealth());
	}
}

void APlayerCharacter::StaminaChangedCallback(const FOnAttributeChangeData& Data)
{
	if (AttributeSet)
	{
		OnStaminaChanged.Broadcast(Data.NewValue, AttributeSet->GetMaxStamina());

		if (Data.NewValue <= 0.0f)
		{
			OnSprintInput(false);
		}

		if (Data.NewValue < Data.OldValue)
		{
			if (ActiveStaminaRegenHandle.IsValid())
			{
				AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveStaminaRegenHandle);
				ActiveStaminaRegenHandle.Invalidate();
			}

			GetWorld()->GetTimerManager().ClearTimer(StaminaRegenTimerHandle);
			GetWorld()->GetTimerManager().SetTimer(StaminaRegenTimerHandle, this, &APlayerCharacter::StartStaminaRegen, StaminaRegenDelay, false);
		}
		else if (Data.NewValue >= AttributeSet->GetMaxStamina())
		{
			if (ActiveStaminaRegenHandle.IsValid())
			{
				AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveStaminaRegenHandle);
				ActiveStaminaRegenHandle.Invalidate();
			}
			GetWorld()->GetTimerManager().ClearTimer(StaminaRegenTimerHandle);
		}
	}
}

void APlayerCharacter::MentalityChangedCallback(const FOnAttributeChangeData& Data)
{
	if (AttributeSet)
	{
		OnMentalityChanged.Broadcast(Data.NewValue, AttributeSet->GetMaxMentality());
	}
}

