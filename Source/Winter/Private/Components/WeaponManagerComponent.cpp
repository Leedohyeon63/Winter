#include "Components/WeaponManagerComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Actor/WeaponProjectile.h"
#include "Components/PlayerInventoryComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameplayEffect.h"
#include "WinterGameplayTags.h"

UWeaponManagerComponent::UWeaponManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWeaponManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	InventoryComponent = GetOwner()
		? GetOwner()->FindComponentByClass<UPlayerInventoryComponent>()
		: nullptr;

	if (InventoryComponent)
	{
		// [웨폰 매니저 추가] 장착/해제와 레벨 이동 복원 후 활성 무기를 자동으로 다시 확인한다.
		InventoryComponent->OnInventoryChanged.AddDynamic(
			this,
			&UWeaponManagerComponent::HandleInventoryChanged);
	}

	RefreshActiveWeapon();
}

void UWeaponManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.RemoveDynamic(
			this,
			&UWeaponManagerComponent::HandleInventoryChanged);
	}

	Super::EndPlay(EndPlayReason);
}

bool UWeaponManagerComponent::StartAttack()
{
	RefreshActiveWeapon();

	UItemDefinitionDataAsset* WeaponDefinition = GetActiveWeapon();
	UWorld* World = GetWorld();
	if (!WeaponDefinition
		|| !World
		|| !WeaponDefinition->AttackTypeTag.IsValid()
		|| !WeaponDefinition->AttackDamageEffect
		|| PendingAttackWeapon)
	{
		return false;
	}

	const FGameplayTag& AttackType = WeaponDefinition->AttackTypeTag;
	const bool bIsMelee = AttackType.MatchesTagExact(WinterGameplayTags::Weapon_AttackType_Melee);
	const bool bIsHitscan = AttackType.MatchesTagExact(WinterGameplayTags::Weapon_AttackType_Hitscan);
	const bool bIsProjectile = AttackType.MatchesTagExact(WinterGameplayTags::Weapon_AttackType_Projectile);
	if ((!bIsMelee && !bIsHitscan && !bIsProjectile)
		|| (bIsProjectile && !WeaponDefinition->ProjectileClass))
	{
		// [웨폰 매니저 추가] 잘못 설정된 무기는 쿨다운과 몽타주가 시작되기 전에 거부한다.
		return false;
	}

	const float CurrentTime = World->GetTimeSeconds();
	if (CurrentTime < NextAttackAllowedTime)
	{
		return false;
	}

	PendingAttackWeapon = WeaponDefinition;
	NextAttackAllowedTime = CurrentTime + FMath::Max(0.0f, WeaponDefinition->AttackCooldown);

	ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());
	const float MontageDuration = CharacterOwner && WeaponDefinition->AttackMontage
		? CharacterOwner->PlayAnimMontage(WeaponDefinition->AttackMontage)
		: 0.0f;

	// [웨폰 매니저 추가] 유효한 몽타주와 Notify 설정이 모두 있을 때만 판정을 대기한다.
	if (WeaponDefinition->bExecuteAttackOnAnimNotify && MontageDuration > 0.0f)
	{
		return true;
	}

	return ExecutePendingAttack();
}

bool UWeaponManagerComponent::ExecutePendingAttack()
{
	UItemDefinitionDataAsset* WeaponDefinition = PendingAttackWeapon.Get();
	PendingAttackWeapon = nullptr;

	if (!WeaponDefinition)
	{
		return false;
	}

	const FGameplayTag& AttackType = WeaponDefinition->AttackTypeTag;
	if (AttackType.MatchesTagExact(WinterGameplayTags::Weapon_AttackType_Melee))
	{
		return ExecuteMeleeAttack(WeaponDefinition);
	}

	if (AttackType.MatchesTagExact(WinterGameplayTags::Weapon_AttackType_Hitscan))
	{
		return ExecuteHitscanAttack(WeaponDefinition);
	}

	if (AttackType.MatchesTagExact(WinterGameplayTags::Weapon_AttackType_Projectile))
	{
		return ExecuteProjectileAttack(WeaponDefinition);
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("WeaponManager: 지원하지 않는 AttackTypeTag입니다: %s"),
		*AttackType.ToString());
	return false;
}

bool UWeaponManagerComponent::SwitchWeapon()
{
	const EEquipmentSlot OtherSlot = ActiveWeaponSlot == EEquipmentSlot::PrimaryWeapon
		? EEquipmentSlot::SecondaryWeapon
		: EEquipmentSlot::PrimaryWeapon;

	return SetActiveWeaponSlot(OtherSlot);
}

bool UWeaponManagerComponent::SetActiveWeaponSlot(EEquipmentSlot NewSlot)
{
	if (!InventoryComponent
		|| (NewSlot != EEquipmentSlot::PrimaryWeapon
			&& NewSlot != EEquipmentSlot::SecondaryWeapon)
		|| !InventoryComponent->GetEquippedItem(NewSlot))
	{
		return false;
	}

	if (ActiveWeaponSlot == NewSlot)
	{
		return true;
	}

	// [웨폰 매니저 추가] 무기 전환 시 이전 무기의 대기 중 AnimNotify 공격이 실행되지 않도록 취소한다.
	PendingAttackWeapon = nullptr;
	ActiveWeaponSlot = NewSlot;
	RefreshActiveWeapon();
	return true;
}

UItemDefinitionDataAsset* UWeaponManagerComponent::GetActiveWeapon() const
{
	return InventoryComponent
		? InventoryComponent->GetEquippedItem(ActiveWeaponSlot)
		: nullptr;
}

void UWeaponManagerComponent::HandleInventoryChanged()
{
	RefreshActiveWeapon();
}

void UWeaponManagerComponent::RefreshActiveWeapon()
{
	if (!InventoryComponent)
	{
		return;
	}

	UItemDefinitionDataAsset* CurrentWeapon = InventoryComponent->GetEquippedItem(ActiveWeaponSlot);
	if (!CurrentWeapon)
	{
		const EEquipmentSlot OtherSlot = ActiveWeaponSlot == EEquipmentSlot::PrimaryWeapon
			? EEquipmentSlot::SecondaryWeapon
			: EEquipmentSlot::PrimaryWeapon;
		if (InventoryComponent->GetEquippedItem(OtherSlot))
		{
			ActiveWeaponSlot = OtherSlot;
			CurrentWeapon = InventoryComponent->GetEquippedItem(OtherSlot);
		}
	}

	if (PendingAttackWeapon && PendingAttackWeapon != CurrentWeapon)
	{
		PendingAttackWeapon = nullptr;
	}

	if (LastActiveWeapon != CurrentWeapon)
	{
		LastActiveWeapon = CurrentWeapon;
		OnActiveWeaponChanged.Broadcast(ActiveWeaponSlot, CurrentWeapon);
	}
}

bool UWeaponManagerComponent::ExecuteMeleeAttack(UItemDefinitionDataAsset* WeaponDefinition)
{
	ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());
	UWorld* World = GetWorld();
	if (!CharacterOwner || !World || !WeaponDefinition || !WeaponDefinition->AttackDamageEffect)
	{
		return false;
	}

	const FVector Forward = CharacterOwner->GetActorForwardVector();
	const FVector Start = CharacterOwner->GetActorLocation()
		+ FVector::UpVector * CharacterOwner->GetSimpleCollisionHalfHeight() * 0.5f;
	const FVector End = Start + Forward * FMath::Max(0.0f, WeaponDefinition->AttackRange);
	const float Radius = FMath::Max(1.0f, WeaponDefinition->AttackRadius);

	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WeaponMeleeSweep), false, CharacterOwner);
	QueryParams.AddIgnoredActor(CharacterOwner);

	TArray<FHitResult> HitResults;
	World->SweepMultiByObjectType(
		HitResults,
		Start,
		End,
		FQuat::Identity,
		ObjectQuery,
		FCollisionShape::MakeSphere(Radius),
		QueryParams);

	if (bDrawDebugTraces)
	{
		DrawDebugLine(World, Start, End, FColor::Orange, false, 1.0f, 0, 2.0f);
		DrawDebugSphere(World, Start, Radius, 16, FColor::Orange, false, 1.0f);
		DrawDebugSphere(World, End, Radius, 16, FColor::Orange, false, 1.0f);
	}

	TSet<AActor*> DamagedActors;
	bool bAppliedAnyDamage = false;
	for (const FHitResult& HitResult : HitResults)
	{
		AActor* TargetActor = HitResult.GetActor();
		if (!IsValid(TargetActor) || DamagedActors.Contains(TargetActor))
		{
			continue;
		}

		AController* OwnerController = CharacterOwner->GetController();
		if (OwnerController && !OwnerController->LineOfSightTo(TargetActor))
		{
			continue;
		}

		DamagedActors.Add(TargetActor);
		bAppliedAnyDamage |= ApplyDamageEffect(TargetActor, WeaponDefinition, &HitResult);
	}

	return bAppliedAnyDamage;
}

bool UWeaponManagerComponent::ExecuteHitscanAttack(UItemDefinitionDataAsset* WeaponDefinition)
{
	APawn* PawnOwner = Cast<APawn>(GetOwner());
	UWorld* World = GetWorld();
	if (!PawnOwner || !World || !WeaponDefinition || !WeaponDefinition->AttackDamageEffect)
	{
		return false;
	}

	FVector Start = PawnOwner->GetActorLocation();
	FRotator ViewRotation = PawnOwner->GetActorRotation();
	if (AController* OwnerController = PawnOwner->GetController())
	{
		OwnerController->GetPlayerViewPoint(Start, ViewRotation);
	}

	const FVector End = Start
		+ ViewRotation.Vector() * FMath::Max(0.0f, WeaponDefinition->AttackRange);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WeaponHitscan), true, PawnOwner);
	QueryParams.AddIgnoredActor(PawnOwner);

	FHitResult HitResult;
	const bool bHit = World->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		QueryParams);

	if (bDrawDebugTraces)
	{
		DrawDebugLine(
			World,
			Start,
			bHit ? HitResult.ImpactPoint : End,
			bHit ? FColor::Green : FColor::Red,
			false,
			1.0f,
			0,
			2.0f);
	}

	return bHit && ApplyDamageEffect(HitResult.GetActor(), WeaponDefinition, &HitResult);
}

bool UWeaponManagerComponent::ExecuteProjectileAttack(UItemDefinitionDataAsset* WeaponDefinition)
{
	ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner());
	UWorld* World = GetWorld();
	if (!CharacterOwner
		|| !World
		|| !WeaponDefinition
		|| !WeaponDefinition->ProjectileClass
		|| !WeaponDefinition->AttackDamageEffect)
	{
		return false;
	}

	FVector SpawnLocation = CharacterOwner->GetActorLocation()
		+ CharacterOwner->GetActorForwardVector() * 100.0f;
	FRotator SpawnRotation = CharacterOwner->GetActorRotation();

	if (USkeletalMeshComponent* CharacterMesh = CharacterOwner->GetMesh())
	{
		if (CharacterMesh->DoesSocketExist(WeaponDefinition->ProjectileSocketName))
		{
			SpawnLocation = CharacterMesh->GetSocketLocation(WeaponDefinition->ProjectileSocketName);
		}
	}

	if (AController* OwnerController = CharacterOwner->GetController())
	{
		FVector ViewLocation;
		OwnerController->GetPlayerViewPoint(ViewLocation, SpawnRotation);
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = CharacterOwner;
	SpawnParameters.Instigator = CharacterOwner;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AWeaponProjectile* Projectile = World->SpawnActor<AWeaponProjectile>(
		WeaponDefinition->ProjectileClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParameters);

	if (!Projectile)
	{
		return false;
	}

	IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(CharacterOwner);
	Projectile->InitializeProjectile(
		CharacterOwner,
		AbilityInterface ? AbilityInterface->GetAbilitySystemComponent() : nullptr,
		WeaponDefinition->AttackDamageEffect,
		WeaponDefinition,
		WeaponDefinition->ProjectileSpeed,
		WeaponDefinition->ProjectileLifeSeconds);
	return true;
}

bool UWeaponManagerComponent::ApplyDamageEffect(
	AActor* TargetActor,
	UItemDefinitionDataAsset* WeaponDefinition,
	const FHitResult* HitResult) const
{
	if (!IsValid(TargetActor) || TargetActor == GetOwner() || !WeaponDefinition)
	{
		return false;
	}

	IAbilitySystemInterface* SourceInterface = Cast<IAbilitySystemInterface>(GetOwner());
	IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(TargetActor);
	UAbilitySystemComponent* SourceAbilitySystem = SourceInterface
		? SourceInterface->GetAbilitySystemComponent()
		: nullptr;
	UAbilitySystemComponent* TargetAbilitySystem = TargetInterface
		? TargetInterface->GetAbilitySystemComponent()
		: nullptr;

	if (!SourceAbilitySystem || !TargetAbilitySystem || !WeaponDefinition->AttackDamageEffect)
	{
		return false;
	}

	FGameplayEffectContextHandle EffectContext = SourceAbilitySystem->MakeEffectContext();
	EffectContext.AddInstigator(GetOwner(), GetOwner());
	EffectContext.AddSourceObject(WeaponDefinition);
	if (HitResult)
	{
		EffectContext.AddHitResult(*HitResult, true);
	}

	const FGameplayEffectSpecHandle SpecHandle = SourceAbilitySystem->MakeOutgoingSpec(
		WeaponDefinition->AttackDamageEffect,
		1.0f,
		EffectContext);
	if (!SpecHandle.IsValid())
	{
		return false;
	}

	TargetAbilitySystem->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	return true;
}
