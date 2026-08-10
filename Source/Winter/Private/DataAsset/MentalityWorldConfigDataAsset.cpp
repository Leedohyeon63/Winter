#include "DataAsset/MentalityWorldConfigDataAsset.h"

FPrimaryAssetId UMentalityWorldConfigDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("MentalityWorldConfig"), GetFName());
}

const FMentalityWorldStateProfile* UMentalityWorldConfigDataAsset::FindProfileByState(
	EMentalityWorldState State) const
{
	return StateProfiles.FindByPredicate(
		[State](const FMentalityWorldStateProfile& Profile)
		{
			return Profile.State == State;
		});
}

const FMentalityWorldStateProfile* UMentalityWorldConfigDataAsset::FindProfileForPercent(
	float MentalityPercent) const
{
	const float ClampedPercent = FMath::Clamp(MentalityPercent, 0.0f, 100.0f);
	const FMentalityWorldStateProfile* BestProfile = nullptr;

	for (const FMentalityWorldStateProfile& Profile : StateProfiles)
	{
		if (ClampedPercent < Profile.MinimumMentalityPercent)
		{
			continue;
		}

		if (!BestProfile
			|| Profile.MinimumMentalityPercent > BestProfile->MinimumMentalityPercent)
		{
			BestProfile = &Profile;
		}
	}

	return BestProfile;
}
