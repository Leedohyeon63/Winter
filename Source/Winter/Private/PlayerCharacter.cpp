// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerCharacter.h"
#include "AbilitySystemComponent.h"
#include "Attribute/PlayerStatAttributeSet.h" 
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/InteractableComponent.h"
#include "Components/PlayerInventoryComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameState/MainGameState.h"
#include "Subsystem/LevelTravelSubsystem.h"

// Sets default values
APlayerCharacter::APlayerCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UPlayerStatAttributeSet>(TEXT("AttributeSet"));
	InventoryComponent = CreateDefaultSubobject<UPlayerInventoryComponent>(TEXT("InventoryComponent"));
}

UAbilitySystemComponent* APlayerCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// Called when the game starts or when spawned
void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}

	if (AbilitySystemComponent && AttributeSet)
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute()).AddUObject(this, &APlayerCharacter::HealthChangedCallback);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetStaminaAttribute()).AddUObject(this, &APlayerCharacter::StaminaChangedCallback);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMentalityAttribute()).AddUObject(this, &APlayerCharacter::MentalityChangedCallback);

		OnHealthChanged.Broadcast(AttributeSet->GetHealth(), AttributeSet->GetMaxHealth());
		OnStaminaChanged.Broadcast(AttributeSet->GetStamina(), AttributeSet->GetMaxStamina());
		OnMentalityChanged.Broadcast(AttributeSet->GetMentality(), AttributeSet->GetMaxMentality());
	}

	// [레벨 이동 추가] 새 Pawn의 BeginPlay/빙의 순서 영향을 피하려고 다음 틱에 상태와 도착 위치를 복원한다.
	GetWorldTimerManager().SetTimerForNextTick(this, &APlayerCharacter::RestoreAfterLevelTravel);
}

FPlayerTravelState APlayerCharacter::CaptureTravelState() const
{
	FPlayerTravelState State;
	if (!AttributeSet || !InventoryComponent)
	{
		return State;
	}

	// [레벨 이동 추가] 현재 GAS 속성값과 장비 포함 인벤토리를 하나의 스냅샷으로 만든다.
	State.Health = AttributeSet->GetHealth();
	State.MaxHealth = AttributeSet->GetMaxHealth();
	State.Stamina = AttributeSet->GetStamina();
	State.MaxStamina = AttributeSet->GetMaxStamina();
	State.Mentality = AttributeSet->GetMentality();
	State.MaxMentality = AttributeSet->GetMaxMentality();
	State.Inventory = InventoryComponent->CaptureTravelState();
	State.bIsValid = true;
	return State;
}

void APlayerCharacter::RestoreTravelState(const FPlayerTravelState& InState)
{
	if (!InState.bIsValid || !AbilitySystemComponent || !AttributeSet || !InventoryComponent)
	{
		return;
	}

	// [레벨 이동 추가] 최대값을 먼저 복원한 뒤 현재값을 제한해 잘못된 속성 범위를 막는다.
	AbilitySystemComponent->SetNumericAttributeBase(AttributeSet->GetMaxHealthAttribute(), InState.MaxHealth);
	AbilitySystemComponent->SetNumericAttributeBase(AttributeSet->GetHealthAttribute(), FMath::Clamp(InState.Health, 0.0f, InState.MaxHealth));
	AbilitySystemComponent->SetNumericAttributeBase(AttributeSet->GetMaxStaminaAttribute(), InState.MaxStamina);
	AbilitySystemComponent->SetNumericAttributeBase(AttributeSet->GetStaminaAttribute(), FMath::Clamp(InState.Stamina, 0.0f, InState.MaxStamina));
	AbilitySystemComponent->SetNumericAttributeBase(AttributeSet->GetMaxMentalityAttribute(), InState.MaxMentality);
	AbilitySystemComponent->SetNumericAttributeBase(AttributeSet->GetMentalityAttribute(), FMath::Clamp(InState.Mentality, 0.0f, InState.MaxMentality));

	InventoryComponent->RestoreTravelState(InState.Inventory);

	// [레벨 이동 추가] HUD가 초기 브로드캐스트 이후 생성됐거나 바인딩됐더라도 최종 복원값을 받게 한다.
	OnHealthChanged.Broadcast(AttributeSet->GetHealth(), AttributeSet->GetMaxHealth());
	OnStaminaChanged.Broadcast(AttributeSet->GetStamina(), AttributeSet->GetMaxStamina());
	OnMentalityChanged.Broadcast(AttributeSet->GetMentality(), AttributeSet->GetMaxMentality());
}

void APlayerCharacter::RestoreAfterLevelTravel()
{
	// [레벨 이동 추가] GameInstanceSubsystem은 OpenLevel에도 유지되므로 새 Pawn에 스냅샷을 적용할 수 있다.
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	if (ULevelTravelSubsystem* TravelSubsystem = GameInstance
		? GameInstance->GetSubsystem<ULevelTravelSubsystem>()
		: nullptr)
	{
		TravelSubsystem->RestorePlayerAfterTravel(this);
	}

	// [멘탈리티 월드 상태 추가] 복원이 끝난 값을 사용하고, 새 레벨 구독자를 위해 같은 상태도 다시 알린다.
	UpdateMentalityWorldState(true);
}

void APlayerCharacter::UpdateMentalityWorldState(bool bForceBroadcast)
{
	if (!AttributeSet || !GetWorld())
	{
		return;
	}

	AMainGameState* MainGameState = GetWorld()->GetGameState<AMainGameState>();
	if (!MainGameState)
	{
		return;
	}

	// [멘탈리티 월드 상태 추가] Player는 원본 수치만 전달하고 상태 판정은 MainGameState 한 곳에서 수행한다.
	MainGameState->UpdateMentalityWorldState(
		AttributeSet->GetMentality(),
		AttributeSet->GetMaxMentality(),
		bForceBroadcast);
}

// Called every frame
void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	CheckCrosshairHover();
}

// Called to bind functionality to input
void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
			{
				if (DefaultMappingContext)
				{
					InputSubsystem->AddMappingContext(DefaultMappingContext, DefaultMappingPriority);
				}
			}
		}
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &APlayerCharacter::TryInteract);
		}

		if (InventoryAction)
		{
			EnhancedInputComponent->BindAction(
				InventoryAction,
				ETriggerEvent::Started,
				this,
				&APlayerCharacter::RequestInventoryToggle);
		}
	}
}

void APlayerCharacter::OnSprintInput(bool bIsSprinting)
{
	if (bIsSprinting)
	{

		if (AttributeSet && AttributeSet->GetStamina() <= 0.0f)
		{
			return;
		}

		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;

		if (AbilitySystemComponent && StaminaDrainEffect && !ActiveStaminaDrainHandle.IsValid())
		{
			FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
			EffectContext.AddInstigator(this, this);

			FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(StaminaDrainEffect, 1.0f, EffectContext);

			if (SpecHandle.IsValid())
			{
				ActiveStaminaDrainHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

		if (AbilitySystemComponent && ActiveStaminaDrainHandle.IsValid())
		{
			AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveStaminaDrainHandle);
			ActiveStaminaDrainHandle.Invalidate();
		}
	}
}

void APlayerCharacter::StartStaminaRegen()
{
	if (AbilitySystemComponent && StaminaRegenEffect && !ActiveStaminaRegenHandle.IsValid())
	{
		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
		EffectContext.AddInstigator(this, this);
		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(StaminaRegenEffect, 1.0f, EffectContext);

		if (SpecHandle.IsValid())
		{
			ActiveStaminaRegenHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}

void APlayerCharacter::TryInteract()
{
	CheckCrosshairHover();

	if (IsValid(CurrentHoveredComponent) && CurrentHoveredComponent->CanInteract(this))
	{
		if (CurrentHoveredComponent->Interact(this))
		{
			CheckCrosshairHover();
		}
	}
}

void APlayerCharacter::RequestInventoryToggle()
{
	OnInventoryToggleRequested.Broadcast();
}

void APlayerCharacter::CheckCrosshairHover()
{
	if (!Controller)
	{
		return;
	}

	FVector StartLoc;
	FRotator CamRot;
	Controller->GetPlayerViewPoint(StartLoc, CamRot);

	FVector EndLoc = StartLoc + (CamRot.Vector() * InteractRange);

	FHitResult HitResult;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, StartLoc, EndLoc, ECC_Visibility, CollisionParams);

	//DrawDebugLine(GetWorld(), StartLoc, EndLoc, bHit ? FColor::Green : FColor::Red, false, -1.0f, 0, 2.0f);

	UInteractableComponent* FoundComp = nullptr;

	if (bHit && HitResult.GetActor())
	{
		UInteractableComponent* InteractComp = HitResult.GetActor()->FindComponentByClass<UInteractableComponent>();

		if (InteractComp && InteractComp->CanInteract(this))
		{
			FoundComp = InteractComp;
		}
	}

	if (FoundComp != CurrentHoveredComponent)
	{
		CurrentHoveredComponent = FoundComp;

		if (CurrentHoveredComponent)
		{
			//GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Cyan, TEXT("C++ sign com"));
			OnHoverInteractableChanged.Broadcast(true, CurrentHoveredComponent->PromptText);
		}
		else
		{
			OnHoverInteractableChanged.Broadcast(false, FString());
		}
	}
}

void APlayerCharacter::HealthChangedCallback(const FOnAttributeChangeData& Data)
{
	if (AttributeSet)
	{
		OnHealthChanged.Broadcast(Data.NewValue, AttributeSet->GetMaxHealth());
	}
}

void APlayerCharacter::StaminaChangedCallback(const FOnAttributeChangeData& Data)
{
	if (AttributeSet)
	{
		OnStaminaChanged.Broadcast(Data.NewValue, AttributeSet->GetMaxStamina());

		if (Data.NewValue <= 0.0f)
		{
			OnSprintInput(false);
		}

		if (Data.NewValue < Data.OldValue)
		{
			if (ActiveStaminaRegenHandle.IsValid())
			{
				AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveStaminaRegenHandle);
				ActiveStaminaRegenHandle.Invalidate();
			}

			GetWorld()->GetTimerManager().ClearTimer(StaminaRegenTimerHandle);
			GetWorld()->GetTimerManager().SetTimer(StaminaRegenTimerHandle, this, &APlayerCharacter::StartStaminaRegen, StaminaRegenDelay, false);
		}
		else if (Data.NewValue >= AttributeSet->GetMaxStamina())
		{
			if (ActiveStaminaRegenHandle.IsValid())
			{
				AbilitySystemComponent->RemoveActiveGameplayEffect(ActiveStaminaRegenHandle);
				ActiveStaminaRegenHandle.Invalidate();
			}
			GetWorld()->GetTimerManager().ClearTimer(StaminaRegenTimerHandle);
		}
	}
}

void APlayerCharacter::MentalityChangedCallback(const FOnAttributeChangeData& Data)
{
	if (AttributeSet)
	{
		OnMentalityChanged.Broadcast(Data.NewValue, AttributeSet->GetMaxMentality());

		// [멘탈리티 월드 상태 추가] GAS 값이 바뀔 때 중앙 계층이 새 단계를 계산하도록 요청한다.
		UpdateMentalityWorldState(false);
	}
}

