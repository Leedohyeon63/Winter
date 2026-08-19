// Fill out your copyright notice in the Description page of Project Settings.


#include "WinterGameplayTags.h"

namespace WinterGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(Weapon_AttackType_Melee,"Weapon.AttackType.Melee");

	UE_DEFINE_GAMEPLAY_TAG(Weapon_AttackType_Hitscan,"Weapon.AttackType.Hitscan");

	UE_DEFINE_GAMEPLAY_TAG(Weapon_AttackType_Projectile,"Weapon.AttackType.Projectile");

	// [공통 데미지 처리 추가] 무기와 몬스터가 같은 이름으로 피해량을 GameplayEffect에 전달한다.
	UE_DEFINE_GAMEPLAY_TAG(Data_Damage, "Data.Damage");

	// [사망 상태 추가] 플레이어와 몬스터가 공통으로 사용하는 사망 상태다.
	UE_DEFINE_GAMEPLAY_TAG(State_Dead, "State.Dead");
}
