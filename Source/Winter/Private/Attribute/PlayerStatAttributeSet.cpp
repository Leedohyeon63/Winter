// Fill out your copyright notice in the Description page of Project Settings.


#include "Attribute/PlayerStatAttributeSet.h"
#include "GameplayEffectExtension.h"

UPlayerStatAttributeSet::UPlayerStatAttributeSet()
{
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitStamina(100.f);
	InitMaxStamina(100.f);
	InitMentality(100.f);
	InitMaxMentality(100.f);
}

void UPlayerStatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));

		if (GetHealth() <= 0.0f)
		{
			//사망
		}
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
