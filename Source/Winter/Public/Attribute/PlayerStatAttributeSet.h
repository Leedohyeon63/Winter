// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "PlayerStatAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)
/**
 * 
 */
UCLASS()
class WINTER_API UPlayerStatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
public:
	UPlayerStatAttributeSet();

	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;


	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(USurvivalAttributeSet, Health)

		UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(USurvivalAttributeSet, MaxHealth)


		UPROPERTY(BlueprintReadOnly, Category = "Attributes|Stamina")
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(USurvivalAttributeSet, Stamina)

		UPROPERTY(BlueprintReadOnly, Category = "Attributes|Stamina")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(USurvivalAttributeSet, MaxStamina)


		UPROPERTY(BlueprintReadOnly, Category = "Attributes|Mentality")
	FGameplayAttributeData Mentality;
	ATTRIBUTE_ACCESSORS(USurvivalAttributeSet, Mentality)

		UPROPERTY(BlueprintReadOnly, Category = "Attributes|Mentality")
	FGameplayAttributeData MaxMentality;
	ATTRIBUTE_ACCESSORS(USurvivalAttributeSet, MaxMentality)
};
