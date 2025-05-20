// Fill out your copyright notice in the Description page of Project Settings.


#include "RGAssetManager.h"

#include "RGGameData.h"
#include "RandomGod/Charactor/RGPawnData.h"
#include "BattleGamePlayLibrary/BattleLog/BattleLogConfig.h"
#include "BattleGameplayLibrary/BattleLog/BattleLogRG.h"

const FName FRGBundles::Equipped("Equipped");

//////////////////////////////////////////////////////////////////////

static FAutoConsoleCommand CVarDumpLoadedAssets(
	TEXT("RG.DumpLoadedAssets"),
	TEXT("Shows all assets that were loaded via the asset manager and are currently in memory."),
	FConsoleCommandDelegate::CreateStatic(URGAssetManager::DumpLoadedAssets)
);

#define STARTUP_JOB_WEIGHTED(JobFunc, JobWeight) StartupJobs.Add(FRGAssetManagerStartupJob(#JobFunc, [this](const FRGAssetManagerStartupJob& StartupJob, TSharedPtr<FStreamableHandle>& LoadHandle){JobFunc;}, JobWeight))
#define STARTUP_JOB(JobFunc) STARTUP_JOB_WEIGHTED(JobFunc, 1.f)


URGAssetManager::URGAssetManager()
{
}

URGAssetManager& URGAssetManager::Get()
{
	check(GEngine);

	if (URGAssetManager* Singleton = Cast<URGAssetManager>(GEngine->AssetManager))
	{
		return *Singleton;
	}

	LOG_CONFIG("Invalid AssetManagerClassName in DefaultEngine.ini.  It must be set to LyraAssetManager!")

	// Fatal error above prevents this from being called.
	return *NewObject<URGAssetManager>();
}

void URGAssetManager::DumpLoadedAssets()
{
	LOG_RG("========== Start Dumping Loaded Assets ==========")

	for (const UObject* LoadedAsset : Get().LoadedAssets)
	{
		LOG_RG("  %s", *GetNameSafe(LoadedAsset))
	}
	LOG_RG("... %d assets in loaded pool", Get().LoadedAssets.Num())
	LOG_RG("========== Finish Dumping Loaded Assets ==========")
}

const URGGameData& URGAssetManager::GetGameData()
{
	return GetOrLoadTypedGameData<URGGameData>(RGGameDataPath);
}

const URGPawnData* URGAssetManager::GetDefaultPawnData() const
{
	return GetAsset(DefaultPawnData);
}

UObject* URGAssetManager::SynchronousLoadAsset(const FSoftObjectPath& AssetPath)
{
	if (AssetPath.IsValid())
	{
		TUniquePtr<FScopeLogTime> LogTimePtr;

		if (ShouldLogAssetLoads())
		{
			LogTimePtr = MakeUnique<FScopeLogTime>(*FString::Printf(TEXT("Synchronously loaded asset [%s]"), *AssetPath.ToString()), nullptr, FScopeLogTime::ScopeLog_Seconds);
		}

		if (UAssetManager::IsInitialized())
		{
			return UAssetManager::GetStreamableManager().LoadSynchronous(AssetPath, false);
		}

		// Use LoadObject if asset manager isn't ready yet.
		return AssetPath.TryLoad();
	}

	return nullptr;
}

bool URGAssetManager::ShouldLogAssetLoads()
{
	static bool bLogAssetLoads = FParse::Param(FCommandLine::Get(), TEXT("LogAssetLoads"));
	return bLogAssetLoads;
}

void URGAssetManager::AddLoadedAsset(const UObject* Asset)
{
	if (ensureAlways(Asset))
	{
		FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
		LoadedAssets.Add(Asset);
	}
}

void URGAssetManager::StartInitialLoading()
{
	SCOPED_BOOT_TIMING("URGAssetManager::StartInitialLoading");

	// This does all of the scanning, need to do this now even if loads are deferred
	Super::StartInitialLoading();

	STARTUP_JOB(InitializeGameplayCueManager());

	{
		// Load base game data asset
		STARTUP_JOB_WEIGHTED(GetGameData(), 25.f);
	}

	// Run all the queued up startup jobs
	DoAllStartupJobs();
}

#if WITH_EDITOR
void URGAssetManager::PreBeginPIE(bool bStartSimulate)
{
	Super::PreBeginPIE(bStartSimulate);

	{
		FScopedSlowTask SlowTask(0, NSLOCTEXT("RandomEditor", "BeginLoadingPIEData", "Loading PIE Data"));
		const bool bShowCancelButton = false;
		const bool bAllowInPIE = true;
		SlowTask.MakeDialog(bShowCancelButton, bAllowInPIE);

		const URGGameData& LocalGameDataCommon = GetGameData();

		// Intentionally after GetGameData to avoid counting GameData time in this timer
		SCOPE_LOG_TIME_IN_SECONDS(TEXT("PreBeginPIE asset preloading complete"), nullptr);

		// You could add preloading of anything else needed for the experience we'll be using here
		// (e.g., by grabbing the default experience from the world settings + the experience override in developer settings)
	}
}
#endif


UPrimaryDataAsset* URGAssetManager::LoadGameDataOfClass(TSubclassOf<UPrimaryDataAsset> DataClass,
	const TSoftObjectPtr<UPrimaryDataAsset>& DataClassPath, FPrimaryAssetType PrimaryAssetType)
{
	return nullptr;
}

void URGAssetManager::DoAllStartupJobs()
{
	SCOPED_BOOT_TIMING("URGAssetManager::DoAllStartupJobs");
	const double AllStartupJobsStartTime = FPlatformTime::Seconds();

	if (IsRunningDedicatedServer())
	{
		// No need for periodic progress updates, just run the jobs
		for (const FRGAssetManagerStartupJob& StartupJob : StartupJobs)
		{
			StartupJob.DoJob();
		}
	}
	else
	{
		if (StartupJobs.Num() > 0)
		{
			float TotalJobValue = 0.0f;
			for (const FRGAssetManagerStartupJob& StartupJob : StartupJobs)
			{
				TotalJobValue += StartupJob.JobWeight;
			}

			float AccumulatedJobValue = 0.0f;
			for (FRGAssetManagerStartupJob& StartupJob : StartupJobs)
			{
				const float JobValue = StartupJob.JobWeight;
				StartupJob.SubstepProgressDelegate.BindLambda([This = this, AccumulatedJobValue, JobValue, TotalJobValue](float NewProgress)
					{
						const float SubstepAdjustment = FMath::Clamp(NewProgress, 0.0f, 1.0f) * JobValue;
						const float OverallPercentWithSubstep = (AccumulatedJobValue + SubstepAdjustment) / TotalJobValue;

						This->UpdateInitialGameContentLoadPercent(OverallPercentWithSubstep);
					});

				StartupJob.DoJob();

				StartupJob.SubstepProgressDelegate.Unbind();

				AccumulatedJobValue += JobValue;

				UpdateInitialGameContentLoadPercent(AccumulatedJobValue / TotalJobValue);
			}
		}
		else
		{
			UpdateInitialGameContentLoadPercent(1.0f);
		}
	}

	StartupJobs.Empty();
	LOG_RG("All startup jobs took %.2f seconds to complete",FPlatformTime::Seconds() - AllStartupJobsStartTime)
}

void URGAssetManager::InitializeGameplayCueManager()
{
	// SCOPED_BOOT_TIMING("URGAssetManager::InitializeGameplayCueManager");
	//
	// ULyraGameplayCueManager* GCM = ULyraGameplayCueManager::Get();
	// check(GCM);
	// GCM->LoadAlwaysLoadedCues();
}

void URGAssetManager::UpdateInitialGameContentLoadPercent(float GameContentPercent)
{
}
