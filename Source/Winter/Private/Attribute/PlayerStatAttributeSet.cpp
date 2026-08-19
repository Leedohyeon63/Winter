// Fill out your copyright notice in the Description page of Project Settings.


#include "Attribute/PlayerStatAttributeSet.h"
#include "GameplayEffectExtension.h"

UPlayerStatAttributeSet::UPlayerStatAttributeSet()
{
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitIncomingDamage(0.0f);
	InitStamina(50.f);
	InitMaxStamina(50.f);
	InitMentality(100.f);
	InitMaxMentality(100.f);
}

void UPlayerStatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		// [공통 데미지 처리 추가] 최대 체력 변경 시 현재 체력이 새로운 범위를 벗어나지 않게 한다.
		SetMaxHealth(FMath::Max(1.0f, GetMaxHealth()));
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		// [공통 데미지 처리 추가] 피해는 양수로 전달하고 여기서 한 번만 체력에서 차감한다.
		const float DamageAmount = FMath::Max(0.0f, GetIncomingDamage());
		SetIncomingDamage(0.0f);
		if (DamageAmount > 0.0f)
		{
			SetHealth(FMath::Clamp(GetHealth() - DamageAmount, 0.0f, GetMaxHealth()));
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}

	else if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.0f, GetMaxStamina()));

		if (GetStamina() <= 0.0f)
		{
			//다른곳에 해당 코드가 있어야 할 수 있음 스테미나 0일때
		}
	}

	else if (Data.EvaluatedData.Attribute == GetMentalityAttribute())
	{
		SetMentality(FMath::Clamp(GetMentality(), 0.0f, GetMaxMentality()));

		if (GetMentality() <= 20.0f)
		{
			//이것도 다른곳에 써야 할 수 있음
		}
	}
}
