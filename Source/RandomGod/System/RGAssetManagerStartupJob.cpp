// Copyright Epic Games, Inc. All Rights Reserved.

#include "RGAssetManagerStartupJob.h"

#include "BattleGameplayLibrary/BattleLog/BattleLogRG.h"

TSharedPtr<FStreamableHandle> FRGAssetManagerStartupJob::DoJob() const
{
	const double JobStartTime = FPlatformTime::Seconds();

	TSharedPtr<FStreamableHandle> Handle;
	LOG_RG("Startup job \"%s\" starting",*JobName)
	JobFunc(*this, Handle);

	if (Handle.IsValid())
	{
		Handle->BindUpdateDelegate(FStreamableUpdateDelegate::CreateRaw(this, &FRGAssetManagerStartupJob::UpdateSubstepProgressFromStreamable));
		Handle->WaitUntilComplete(0.0f, false);
		Handle->BindUpdateDelegate(FStreamableUpdateDelegate());
	}
	LOG_RG("Startup job \"%s\" took %.2f seconds to complete",*JobName,FPlatformTime::Seconds() - JobStartTime)
	
	return Handle;
}
