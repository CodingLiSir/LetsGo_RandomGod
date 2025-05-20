#include "BattleLibNativeUtil.h"
#include "BattleGamePlayLibrary/BattleLog/BattleLogTest.h"
#include "Engine/LevelStreaming.h"

FName BattleLibNativeUtil::ConvertFromGamePlayTagName(FName& Name)
{
	FString Left = Name.ToString();
	int32 Index;
	if (Left.FindLastChar('.', Index))
	{
		return FName(Left.Mid(Index + 1));
	}
	else
	{
		return Name;
	}
}

FString BattleLibNativeUtil::GetMapName(const UWorld* WorldContext)
{
	return WorldContext->GetMapName().Mid(WorldContext->StreamingLevelsPrefix.Len());
}

FString BattleLibNativeUtil::NetMode2String(ENetMode NetMode)
{
	switch (NetMode)
	{
	case ENetMode::NM_Standalone:
		return IDENTIFIER_TEXT(ENetMode::NM_Standalone);
	case ENetMode::NM_DedicatedServer:
		return IDENTIFIER_TEXT(ENetMode::NM_DedicatedServer);
	case ENetMode::NM_ListenServer:
		return IDENTIFIER_TEXT(ENetMode::NM_ListenServer);
	case ENetMode::NM_Client:
		return IDENTIFIER_TEXT(ENetMode::NM_Client);
	default:
		return IDENTIFIER_TEXT(ENetMode::NM_MAX);;
	}
}

bool BattleLibNativeUtil::IsClient(UObject* Object)
{
	if (Object)
	{
		ENetMode Mode = Object->GetWorld()->GetNetMode();
		return Mode == NM_Client || Mode == NM_Standalone;
	}
	else
	{
		return false;
	}
}

bool BattleLibNativeUtil::IsClientOrStandalone(UObject* Object)
{
	if (IsClient(Object))
		return true;
	// battle项目特有的判断方式
	// 我们目前不支持Listen Server,唯一会出现ListenServer的就是开启了Replay录制的单机
	ENetMode Mode = Object->GetWorld()->GetNetMode();
	return Mode == NM_Standalone || Mode == NM_ListenServer;
}

bool BattleLibNativeUtil::IsServer(UObject* Object)
{
	if (Object)
	{
		ENetMode Mode = Object->GetWorld()->GetNetMode();
		return Mode == NM_DedicatedServer || Mode == NM_ListenServer;
	}
	else
	{
		return false;
	}
}


bool BattleLibNativeUtil::IsGameWorld(UWorld* World)
{
	return World && (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE);
}

bool BattleLibNativeUtil::PackageIsTargetSoftPath(UPackage* Pkg, const TSoftObjectPtr<UWorld>& WorldPath)
{
	if (IsValid(Pkg) == false)
	{
		ERROR_GAMEPLAY("PackageIsTargetSoftPath Pkg is invalid");
		return false;
	}

	auto&& CurrentLevelPkgName = Pkg->GetLoadedPath().GetPackageFName();
	auto&& TargetName = WorldPath.ToSoftObjectPath().GetLongPackageFName();
	return CurrentLevelPkgName == TargetName;
}

FName BattleLibNativeUtil::GetStreamingLevelPackagePath(ULevelStreaming* Level)
{
	if (Level == nullptr)
	{
		return NAME_None;
	}
	else if (Level->PackageNameToLoad.IsNone())
	{
		return Level->GetWorldAssetPackageFName();
	}
	else
	{
		return Level->PackageNameToLoad;
	}
}

bool BattleLibNativeUtil::IsLevelEqualsSoftPath(UWorld* Level, const TSoftObjectPtr<UWorld>& WorldPath)
{
	if (Level == nullptr)
	{
		ERROR_GAMEPLAY("LevelIsTargetSoftPath Level is invalid");
		return false;
	}

	auto PkgName = FName(Level->GetPackage()->GetLoadedPath().GetPackageName());
	auto&& TargetName = WorldPath.ToSoftObjectPath().GetLongPackageFName();
	// LOG_GAMEPLAY("LevelIsTargetSoftPath PkgName:%s TargetName:%s result =%d", *PkgName.ToString(), *TargetName.ToString(), PkgName == TargetName);
	return PkgName == TargetName;
}

bool BattleLibNativeUtil::IsLevelEqualsSoftPath(ULevelStreaming* Level, const TSoftObjectPtr<UWorld>& WorldPath)
{
	if (Level == nullptr)
	{
		ERROR_GAMEPLAY("LevelIsTargetSoftPath Level is invalid");
		return false;
	}
	auto name_lft =Level->GetWorldAsset().ToSoftObjectPath().GetAssetPath().GetPackageName() ;
	auto name_rht = WorldPath.ToSoftObjectPath().GetAssetPath().GetPackageName();
	return name_lft== name_rht;
}

bool BattleLibNativeUtil::IsLevelEqualsSoftPath(ULevelStreaming* Level, const FName& WorldPath)
{
	if (Level == nullptr)
	{
		ERROR_GAMEPLAY("LevelIsTargetSoftPath Level is invalid");
		return false;
	}

	auto&& CurrentLevelPkgName = GetStreamingLevelPackagePath(Level);
	// LOG_GAMEPLAY("LevelIsTargetSoftPath CurrentLevelPkgName:%s WorldPath:%s result =%d", *CurrentLevelPkgName.ToString(), *WorldPath.ToString(), CurrentLevelPkgName == WorldPath);
	return CurrentLevelPkgName == WorldPath;
}

void BattleLibNativeUtil::PrintLogWithCallStack(const FString& Info, ELogVerbosity::Type PrintLogType, int32 StackCount)
{
	static ANSICHAR* StackMem = (ANSICHAR*)FMemory::SystemMalloc(65536);

	StackMem[0] = 0;

	FPlatformStackWalk::StackWalkAndDumpEx(StackMem, 65536, 1,
	                                       FGenericPlatformStackWalk::EStackWalkFlags::FlagsUsedWhenHandlingEnsure);

	FString StackText(StackMem);

	if (StackCount <= 0)
	{
		if (PrintLogType >= ELogVerbosity::Type::Fatal && PrintLogType <= ELogVerbosity::Type::Error)
		{
			ERROR_TEST("%s", *FString::Printf(TEXT("%s\n%s"),*Info,*StackText))
		}
		else if (PrintLogType == ELogVerbosity::Type::Warning)
		{
			WARN_TEST("%s", *FString::Printf(TEXT("%s\n%s"),*Info,*StackText))
		}
		else
		{
			LOG_TEST("%s", *FString::Printf(TEXT("%s\n%s"),*Info,*StackText))
		}
	}
	else
	{
		TArray<FString> StackArray;
		StackText.ParseIntoArrayLines(StackArray);
		FString FullLog = Info;
		int32 Count = FMath::Min(StackCount, StackArray.Num());
		for (int32 i = 0; i < Count; ++i)
		{
			FullLog += FString::Printf(TEXT("\n%s"), *StackArray[i]);
		}

		if (PrintLogType >= ELogVerbosity::Type::Fatal && PrintLogType <= ELogVerbosity::Type::Error)
		{
			ERROR_TEST("%s", *FullLog)
		}
		else if (PrintLogType == ELogVerbosity::Type::Warning)
		{
			WARN_TEST("%s", *FullLog)
		}
		else
		{
			LOG_TEST("%s", *FullLog)
		}
	}

	// FMemory::SystemFree(StackMem);
}

FName BattleLibNativeUtil::ConvertFromGamePlayTagName(FName&& Name)
{
	FString Left = Name.ToString();
	int32 Index;
	if (Left.FindLastChar('.', Index))
	{
		return FName(Left.Mid(Index + 1));
	}
	else
	{
		return Name;
	}
}
