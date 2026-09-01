#pragma once

#include "CoreMinimal.h"

class AActor;
class UAbilitySystemComponent;
class UGameplayEffect;

/** 플레이어 무기와 몬스터가 동일한 GAS 피해 전달 규칙을 사용하도록 모은 공통 함수다. */
namespace WinterCombat
{
	// [공통 데미지 처리 추가] Data.Damage SetByCaller와 피격 정보를 포함한 GameplayEffect를 대상 ASC에 적용한다.
	WINTER_API bool ApplyDamageEffect(
		UAbilitySystemComponent* SourceAbilitySystem,
		AActor* SourceActor,
		AActor* TargetActor,
		TSubclassOf<UGameplayEffect> DamageEffect,
		float DamageAmount,
		UObject* SourceObject = nullptr,
		const FHitResult* HitResult = nullptr);
}
