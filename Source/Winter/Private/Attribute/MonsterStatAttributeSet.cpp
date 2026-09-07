#include "Attribute/MonsterStatAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Controller.h"
#include "Monster/BaseMonster.h"
#include "Perception/AISense_Damage.h"

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
			const float AppliedDamage = FMath::Min(GetHealth(), DamageAmount);
			ABaseMonster* Monster = Cast<ABaseMonster>(GetOwningActor());
			const FName DamageTag = Monster ? Monster->GetDamagePerceptionTag() : NAME_None;
			SetHealth(FMath::Clamp(GetHealth() - DamageAmount, 0.0f, GetMaxHealth()));
			// GAS는 엔진 TakeDamage를 거치지 않으므로 실제 체력 피해를 Damage Sense에 직접 보고한다.
			if (IsValid(Monster) && Monster->IsActiveMonster() && !Monster->IsDead()
				&& AppliedDamage > 0.0f && DamageTag == Monster->GetDamagePerceptionTag())
			{
				const FGameplayEffectContextHandle& Context = Data.EffectSpec.GetContext();
				AActor* Attacker = Context.GetOriginalInstigator();
				if (AController* AttackerController = Cast<AController>(Attacker))
				{
					Attacker = AttackerController->GetPawn();
				}
				if (!IsValid(Attacker))
				{
					AActor* Causer = Context.GetEffectCauser();
					Attacker = IsValid(Causer) && Causer->GetInstigator() ? Causer->GetInstigator() : Causer;
				}
				if (IsValid(Attacker) && Attacker != Monster)
				{
					const FHitResult* Hit = Context.GetHitResult();
					UAISense_Damage::ReportDamageEvent(Monster, Monster, Attacker, AppliedDamage,
						Attacker->GetActorLocation(), Hit ? FVector(Hit->ImpactPoint) : Monster->GetActorLocation(), DamageTag);
				}
			}
			UE_LOG(LogTemp, Log, TEXT("몬스터 데미지 받음 %f"), DamageAmount);
		}
	}
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
}
