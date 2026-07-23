// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PortalDefinitionDataAsset.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class WINTER_API UPortalDefinitionDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Portal")
    FName PortalId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Portal")
    TSoftObjectPtr<UWorld> TargetLevel;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Portal")
    FName TargetPortalId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Portal")
    FString InteractionPrompt = TEXT("¿Ãµø [F]");
};