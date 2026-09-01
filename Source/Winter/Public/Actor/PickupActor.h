#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupActor.generated.h"

class UInteractableComponent;
class UItemDefinitionDataAsset;
class UStaticMeshComponent;
class USphereComponent;
class USceneComponent;
/** UInteractableComponent를 통해 아이템을 플레이어 인벤토리로 옮기는 월드 액터. */
UCLASS()
class WINTER_API APickupActor : public AActor
{
	GENERATED_BODY()

public:
	APickupActor();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInteractableComponent> InteractableComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup")
	TObjectPtr<UItemDefinitionDataAsset> ItemDefinition;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pickup", meta = (ClampMin = "1"))
	int32 Quantity = 1;

private:
	UFUNCTION()
	void HandleInteract(AActor* Interactor);

	void RefreshPrompt();
};
