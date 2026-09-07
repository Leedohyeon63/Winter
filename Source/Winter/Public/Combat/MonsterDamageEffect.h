#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "MonsterDamageEffect.generated.h"

/** 몬스터끼리의 전투에서 Data.Damage를 몬스터의 피해 속성에 전달한다. */
UCLASS()
class WINTER_API UMonsterDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UMonsterDamageEffect();
};
