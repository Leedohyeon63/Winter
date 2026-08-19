#include "Ani/AnimNotify_ExecuteAttack.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WeaponManagerComponent.h"
#include "Monster/BaseMonster.h"
#include "PlayerCharacter.h"

void UAnimNotify_ExecuteAttack::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* MeshOwner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (APlayerCharacter* PlayerCharacter = Cast<APlayerCharacter>(MeshOwner))
	{
		if (UWeaponManagerComponent* WeaponManager = PlayerCharacter->GetWeaponManagerComponent())
		{
			WeaponManager->ExecutePendingAttack();
		}
		return;
	}

	if (ABaseMonster* Monster = Cast<ABaseMonster>(MeshOwner))
	{
		Monster->ExecutePendingAttack();
	}
}

FString UAnimNotify_ExecuteAttack::GetNotifyName_Implementation() const
{
	return TEXT("Execute Pending Attack");
}
