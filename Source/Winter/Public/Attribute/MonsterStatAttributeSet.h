#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "MonsterStatAttributeSet.generated.h"

#define MONSTER_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class WINTER_API UMonsterStatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UMonsterStatAttributeSet();

	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(BlueprintReadOnly, Category = "Monster|Attributes")
	FGameplayAttributeData Health;
	MONSTER_ATTRIBUTE_ACCESSORS(UMonsterStatAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Monster|Attributes")
	FGameplayAttributeData MaxHealth;
	MONSTER_ATTRIBUTE_ACCESSORS(UMonsterStatAttributeSet, MaxHealth)

	// [공통 데미지 처리 추가] GameplayEffect의 양수 피해를 받은 뒤 Health에서 차감하는 메타 속성이다.
	UPROPERTY(BlueprintReadOnly, Category = "Monster|Attributes")
	FGameplayAttributeData IncomingDamage;
	MONSTER_ATTRIBUTE_ACCESSORS(UMonsterStatAttributeSet, IncomingDamage)
};

