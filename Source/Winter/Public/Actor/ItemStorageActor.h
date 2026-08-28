#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Struct/InventoryTypeStruct.h"
#include "ItemStorageActor.generated.h"

class APlayerController;
class UInteractableComponent;
class UItemStorageWidget;
class UPlayerInventoryComponent;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnItemStorageContentsChangedSignature);


UCLASS()
class WINTER_API AItemStorageActor : public AActor
{
	GENERATED_BODY()

public:
	AItemStorageActor();

	UFUNCTION(BlueprintPure, Category = "Item Storage")
	TArray<FInventoryStack> GetContents() const { return Contents; }

	UFUNCTION(BlueprintPure, Category = "Item Storage")
	FText GetStorageDisplayName() const { return StorageDisplayName; }

	UFUNCTION(BlueprintCallable, Category = "Item Storage")
	int32 TransferStackToInventory(int32 StackIndex, UPlayerInventoryComponent* TargetInventory);

	UPROPERTY(BlueprintAssignable, Category = "Item Storage|Events")
	FOnItemStorageContentsChangedSignature OnContentsChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UInteractableComponent> InteractableComponent;

	/** 저장소를 처음 만났을 때 들어 있을 아이템 목록이다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Storage")
	TArray<FInventoryStack> InitialContents;

	/** 실행 중 실제 남아 있는 아이템 목록이다. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Item Storage")
	TArray<FInventoryStack> Contents;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Storage|UI")
	FText StorageDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Storage|UI")
	TSubclassOf<UItemStorageWidget> StorageWidgetClass;

	/** 같은 맵 안에서 중복되지 않는 ID다. 비어 있으면 배치 액터 이름을 사용한다. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Item Storage|Persistence")
	FName StorageId = NAME_None;

	/** 활성화하면 OpenLevel로 맵을 왕복해도 남은 내용물이 유지된다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Storage|Persistence")
	bool bPersistDuringSession = true;

	/** 모든 아이템을 가져간 저장소의 상호작용을 끈다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Storage|Interaction")
	bool bDisableInteractionWhenEmpty = true;

private:
	UFUNCTION()
	void HandleInteract(AActor* Interactor);

	UFUNCTION()
	void HandleStorageWidgetCloseRequested();

	void LoadOrInitializeContents();
	void NormalizeContents();
	void SaveContentsState();
	void UpdateInteractionState();
	void OpenStorageUI(APlayerController* PlayerController, UPlayerInventoryComponent* PlayerInventory);
	void CloseStorageUI();
	FName BuildStorageStateKey() const;

	UPROPERTY(Transient)
	TObjectPtr<UItemStorageWidget> ActiveStorageWidget;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> ActivePlayerController;
};
