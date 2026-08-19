#include "Combat/WinterCombat.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "WinterGameplayTags.h"

bool WinterCombat::ApplyDamageEffect(
	UAbilitySystemComponent* SourceAbilitySystem,
	AActor* SourceActor,
	AActor* TargetActor,
	TSubclassOf<UGameplayEffect> DamageEffect,
	float DamageAmount,
	UObject* SourceObject,
	const FHitResult* HitResult)
{
	if (!SourceAbilitySystem
		|| !IsValid(SourceActor)
		|| !IsValid(TargetActor)
		|| SourceActor == TargetActor
		|| !DamageEffect
		|| DamageAmount <= 0.0f)
	{
		return false;
	}

	IAbilitySystemInterface* TargetAbilityInterface = Cast<IAbilitySystemInterface>(TargetActor);
	UAbilitySystemComponent* TargetAbilitySystem = TargetAbilityInterface
		? TargetAbilityInterface->GetAbilitySystemComponent()
		: nullptr;
	if (!TargetAbilitySystem
		|| TargetAbilitySystem->HasMatchingGameplayTag(WinterGameplayTags::State_Dead))
	{
		return false;
	}

	FGameplayEffectContextHandle EffectContext = SourceAbilitySystem->MakeEffectContext();
	EffectContext.AddInstigator(SourceActor, SourceActor);
	EffectContext.AddSourceObject(SourceObject ? SourceObject : SourceActor);
	if (HitResult)
	{
		EffectContext.AddHitResult(*HitResult, true);
	}

	FGameplayEffectSpecHandle SpecHandle = SourceAbilitySystem->MakeOutgoingSpec(
		DamageEffect,
		1.0f,
		EffectContext);
	if (!SpecHandle.IsValid())
	{
		return false;
	}

	// [공통 데미지 처리 추가] GE는 이 양수 값을 대상 AttributeSet의 IncomingDamage에 더하도록 설정한다.
	SpecHandle.Data->SetSetByCallerMagnitude(
		WinterGameplayTags::Data_Damage,
		FMath::Max(0.0f, DamageAmount));
	TargetAbilitySystem->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	return true;
}
