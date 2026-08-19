#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_ExecuteAttack.generated.h"

/** 플레이어 무기 또는 몬스터의 대기 중 공격 판정을 몽타주 타이밍에 실행한다. */
UCLASS(meta = (DisplayName = "Execute Pending Attack"))
class WINTER_API UAnimNotify_ExecuteAttack : public UAnimNotify
{
	GENERATED_BODY()

public:
	// [공통 공격 Notify 추가] Notify가 배치된 메시의 소유자를 판별해 알맞은 공격 시스템을 호출한다.
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
