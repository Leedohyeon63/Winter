#include "DataAsset/ItemSpawnTableDataAsset.h"

FPrimaryAssetId UItemSpawnTableDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ItemSpawnTable"), GetFName());
}
