// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "PlayerViewModel.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class WINTER_API UPlayerViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()
public:
	float GetHealth() const { return Health; }
	void SetHealth(float NewHealth){ UE_MVVM_SET_PROPERTY_VALUE(Health, NewHealth); }
	float GetMaxHealth() const { return MaxHealth;}
	void SetMaxHealth(float NewMaxHealth) { UE_MVVM_SET_PROPERTY_VALUE(MaxHealth, NewMaxHealth); }

	float GetStamina() const { return Stamina; }
	void SetStamina(float NewStamina){ UE_MVVM_SET_PROPERTY_VALUE(Stamina, NewStamina);}
	float GetMaxStamina() const { return MaxStamina;}
	void SetMaxStamina(float NewMaxStamina) { UE_MVVM_SET_PROPERTY_VALUE(MaxStamina, NewMaxStamina);}

	float GetMentality() const { return Mentality; }
	void SetMentality(float NewMentality) { UE_MVVM_SET_PROPERTY_VALUE(Mentality, NewMentality); }
	float GetMaxMentality() const { return MaxMentality; }
	void SetMaxMentality(float NewMaxMentality) { UE_MVVM_SET_PROPERTY_VALUE(MaxMentality, NewMaxMentality); }

	float GetHealthPercent() const { return HealthPercent; }
	void SetHealthPercent(float NewPercent) { UE_MVVM_SET_PROPERTY_VALUE(HealthPercent, NewPercent); }

	float GetStaminaPercent() const { return StaminaPercent; }
	void SetStaminaPercent(float NewPercent) { UE_MVVM_SET_PROPERTY_VALUE(StaminaPercent, NewPercent); }

	float GetMentalityPercent() const { return MentalityPercent; }
	void SetMentalityPercent(float NewPercent) { UE_MVVM_SET_PROPERTY_VALUE(MentalityPercent, NewPercent); }
private:
	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess = "true"))
	float Health;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess = "true"))
	float MaxHealth;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess = "true"))
	float Stamina;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess = "true"))
	float MaxStamina;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess = "true"))
	float Mentality;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess = "true"))
	float MaxMentality;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess = "true"))
	float HealthPercent;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess = "true"))
	float StaminaPercent;

	UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess = "true"))
	float MentalityPercent;
};
