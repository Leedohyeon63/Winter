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
	ATTRIBUTE_ACCESSORS(UPlayerStatAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Health")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UPlayerStatAttributeSet, MaxHealth)

	// [공통 데미지 처리 추가] GameplayEffect가 전달한 양수 피해를 잠시 받은 뒤 Health에서 차감하는 메타 속성이다.
	UPROPERTY(BlueprintReadOnly, Category = "Attributes|Combat")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UPlayerStatAttributeSet, IncomingDamage)


		UPROPERTY(BlueprintReadOnly, Category = "Attributes|Stamina")
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UPlayerStatAttributeSet, Stamina)

		UPROPERTY(BlueprintReadOnly, Category = "Attributes|Stamina")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UPlayerStatAttributeSet, MaxStamina)


		UPROPERTY(BlueprintReadOnly, Category = "Attributes|Mentality")
	FGameplayAttributeData Mentality;
	ATTRIBUTE_ACCESSORS(UPlayerStatAttributeSet, Mentality)

		UPROPERTY(BlueprintReadOnly, Category = "Attributes|Mentality")
	FGameplayAttributeData MaxMentality;
	ATTRIBUTE_ACCESSORS(UPlayerStatAttributeSet, MaxMentality)
};
