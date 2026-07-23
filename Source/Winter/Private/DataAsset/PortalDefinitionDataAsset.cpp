// Fill out your copyright notice in the Description page of Project Settings.


#include "DataAsset/PortalDefinitionDataAsset.h"

FPrimaryAssetId UPortalDefinitionDataAsset::GetPrimaryAssetId() const
{
	const FName ResolvedId = PortalId.IsNone() ? GetFName() : PortalId;
	return FPrimaryAssetId(TEXT("PortalDefinition"), ResolvedId);
}
