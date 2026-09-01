#include "Attribute/MonsterStatAttributeSet.h"
#include "GameplayEffectExtension.h"

UMonsterStatAttributeSet::UMonsterStatAttributeSet()
{

	InitHealth(100.0f);
	InitMaxHealth(100.0f);
	InitIncomingDamage(0.0f);
}

void UMonsterStatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		SetMaxHealth(FMath::Max(1.0f, GetMaxHealth()));
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		// [공통 데미지 처리 추가] 피해를 처리한 직후 메타 속성을 0으로 되돌려 중복 차감을 방지한다.
		const float DamageAmount = FMath::Max(0.0f, GetIncomingDamage());
		SetIncomingDamage(0.0f);
		if (DamageAmount > 0.0f)
		{
			SetHealth(FMath::Clamp(GetHealth() - DamageAmount, 0.0f, GetMaxHealth()));
			UE_LOG(LogTemp, Log, TEXT("몬스터 데미지 받음 %f"), DamageAmount);
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
}
