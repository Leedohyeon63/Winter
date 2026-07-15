// Fill out your copyright notice in the Description page of Project Settings.


#include "DataAsset/ItemDefinitionDataAsset.h"

FPrimaryAssetId UItemDefinitionDataAsset::GetPrimaryAssetId() const
{
	const FName ResolvedId = ItemId.IsNone() ? GetFName() : ItemId;
	return FPrimaryAssetId(TEXT("Item"), ResolvedId);
}
