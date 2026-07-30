#include "Attribute/MonsterStatAttributeSet.h"
#include "GameplayEffectExtension.h"

UMonsterStatAttributeSet::UMonsterStatAttributeSet()
{

	InitHealth(100.0f);
	InitMaxHealth(100.0f);
}

void UMonsterStatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		SetMaxHealth(FMath::Max(1.0f, GetMaxHealth()));
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
}

