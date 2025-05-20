#pragma once

#include "BattleGamePlayLibrary/Enums/magic_enum.hpp"
#include "BattleGamePlayLibrary/BattleLog/BattleLogGamePlay.h"
#include "Engine/LevelStreaming.h"
//开放enum class的bit运算
using namespace Next::magic_enum::bitwise_operators;

// 变量名转为常量字符串，方便IDE中重构更名
#define IDENTIFIER_TEXT(v) ((void)v, TEXT(#v))

class BATTLEGAMEPLAYLIBRARY_API BattleLibNativeUtil
{
public:
	static FName ConvertFromGamePlayTagName(FName&& Name);

	static FName ConvertFromGamePlayTagName(FName& Name);

	static inline TMap<UEnum*, TMap<int64, FName>> NameCache;

	static FString GetMapName(const UWorld* WorldContext);

	static FString NetMode2String(ENetMode NetMode);

	static bool IsClient(UObject* Object);

	static bool IsClientOrStandalone(UObject* Object);

	static bool IsServer(UObject* Object);

	template <typename T, class = typename TEnableIf<std::is_enum_v<T>>::type>
	static FName EnumToFName(T EnumValue)
	{
		auto&& enumName = Next::magic_enum::enum_name(EnumValue);
		return FName(enumName.length(), enumName.data());
	}

	template <typename T, class = typename TEnableIf<std::is_enum_v<T>>::type>
	constexpr static decltype(auto) EnumToFName_Constexpr(T EnumValue)
	{
		return Next::magic_enum::enum_name(EnumValue);
	}

	template <typename T>
	static FName EnumIntToFName(int64 Index)
	{
		auto&& Val = Next::magic_enum::enum_value<T>(Index);
		return EnumToFName(Val);
	}

	template <typename T>
	constexpr static ANSICHAR* EnumIntToFName_Constexpr(int64 Index)
	{
		constexpr auto&& Val = Next::magic_enum::enum_value<T>(Index);
		return EnumToFName_Constexpr(Val);
	}

	template <typename T, class = typename TEnableIf<std::is_enum_v<T>>::type>
	constexpr static decltype(auto) EnumNames()
	{
		return Next::magic_enum::enum_names<T>();
	}

	template <typename T, class = typename TEnableIf<std::is_enum_v<T>>::type>
	static TArray<FName> EnumFNames()
	{
		auto&& arr = Next::magic_enum::enum_names<T>();
		TArray<FName> outnames;
		for (auto&& name : arr)
		{
			outnames.Add(FName(name.length(), name.data()));
		}
		return outnames;
	}

	template <typename T, class = typename TEnableIf<std::is_enum_v<T>>::type>
	static TArray<FString> EnumNameStrings()
	{
		auto&& arr = Next::magic_enum::enum_names<T>();
		TArray<FString> outnames;
		for (auto&& name : arr)
		{
			outnames.Add(FString(name.length(), name.data()));
		}
		return outnames;
	}


	template <typename T, class = typename TEnableIf<std::is_enum_v<T>>::type>
	constexpr static decltype(auto) EnumValues()
	{
		return Next::magic_enum::enum_values<T>();
	}

	//-> {{Color::RED, "RED"}, {Color::BLUE, "BLUE"}, {Color::GREEN, "GREEN"}}
	template <typename T, class = typename TEnableIf<std::is_enum_v<T>>::type>
	constexpr static decltype(auto) EnumEntries()
	{
		return Next::magic_enum::enum_entries<T>();
	}

	// #if PLATFORM_LINUX || PLATFORM_LINUXAARCH64
	// PRAGMA_DISABLE_OPTIMIZATION
	// #endif
	template <typename T, class = typename TEnableIf<std::is_enum_v<T>>::type>
	static bool HasEnum(const FName& Name)
	{
		auto&& StrView = Name.ToString();
		auto&& Str = TCHAR_TO_ANSI(*StrView);
		auto Result = Next::magic_enum::enum_cast<T>({Str});
		return Result.has_value();
	}

	template <typename T, class = typename TEnableIf<std::is_enum_v<T>>::type>
	static decltype(auto) ToEnumResult(const FName& Name)
	{
		auto&& StrView = Name.ToString();
		auto&& Str = TCHAR_TO_ANSI(*StrView);
		auto Result = Next::magic_enum::enum_cast<T>({Str});
		return Result;
	}

	template <typename T, class = typename TEnableIf<std::is_enum_v<T>>::type>
	static T ToEnum(const FName& Name)
	{
		auto&& StrView = Name.ToString();
		auto&& Str = TCHAR_TO_ANSI(*StrView);
		auto&& Result = Next::magic_enum::enum_cast<T>({Str});
		if (!Result.has_value())
		{
			ERROR_GAMEPLAY("Not Found Matched Enum By:%s", *Name.ToString())
			return {};
		}

		return Result.value();
	}

	// #if PLATFORM_LINUX || PLATFORM_LINUXAARCH64
	// PRAGMA_ENABLE_OPTIMIZATION
	// #endif

	template <typename T, class = typename TEnableIf<std::is_enum_v<T>>::type>
	constexpr static bool HasEnum(const ANSICHAR* Name)
	{
		return Next::magic_enum::enum_cast<T>({Name}).has_value();
	}

	template <typename T, class = typename TEnableIf<std::is_enum_v<T>>::type>
	constexpr static bool HasEnum(const int Id)
	{
		return Next::magic_enum::enum_cast<T>(Id).has_value();
	}

	template <typename T, class = typename TEnableIf<std::is_enum_v<T>>::type>
	static decltype(auto) ToEnumResult(const ANSICHAR* Name)
	{
		return Next::magic_enum::enum_cast<T>({Name});
	}

	template <typename T, class = typename TEnableIf<std::is_enum_v<T>>::type>
	constexpr static T ToEnum(const ANSICHAR* Name)
	{
		return Next::magic_enum::enum_cast<T>({Name}).value();
	}

	static bool IsGameWorld(UWorld* World);

	/**
	 * @brief 是否目标资源
	 * @param Pkg 
	 * @return 
	 */
	static bool PackageIsTargetSoftPath(UPackage* Pkg, const TSoftObjectPtr<UWorld>& WorldPath);

	/**
	 * @brief 
	 * @param Level 
	 * @return 
	 */
	static FName GetStreamingLevelPackagePath(ULevelStreaming* Level);

	static bool IsLevelEqualsSoftPath(UWorld* Level, const TSoftObjectPtr<UWorld>& WorldPath);

	//是否目标资源
	static bool IsLevelEqualsSoftPath(ULevelStreaming* Level, const TSoftObjectPtr<UWorld>& WorldPath);

	//是否目标资源
	static bool IsLevelEqualsSoftPath(ULevelStreaming* Level, const FName& WorldPath);
	/**
	 * @brief 
	 * @param Info 
	 * @param PrintLogType 
	 * @param StackCount 
	 */
	static void PrintLogWithCallStack(const FString& Info, ELogVerbosity::Type PrintLogType, int32 StackCount = -1);

	template <class T>
	static void CollectActorInLvl(const UWorld* World, TArray<T*>& OutActors)
	{
		for (auto&& Level : World->GetLevels())
		{
			CollectActorInLvl(Level, OutActors);
		}
	}

	template <class T>
	static void CollectActorInLvl(const ULevel* Level, TArray<T*>& OutActors)
	{
		if (IsValid(Level) == false)
		{
			ERROR_GAMEPLAY("Level is nullptr")
			return;
		}
		for (auto&& Actor : Level->Actors)
		{
			// if(IsValid(Actor))
			// {
			// 	LOG_GAMEPLAY("visit actor :%s printname:%s", *Actor->GetName(),*UKismetSystemLibrary::GetDisplayName(Actor))
			// }
			// else
			// {
			// 	LOG_GAMEPLAY("Actor is nullptr")
			// }

			if (auto&& TActor = Cast<T>(Actor))
				OutActors.Add(TActor);
		}
	}

	template <class T>
	static void CollectActorInStreamingLvl(const TArray<ULevelStreaming*>& Levels, TArray<T*>& OutActors)
	{
		for (auto&& Level : Levels)
		{
			if (!Level->IsLevelLoaded())
				continue;
			for (auto&& Actor : Level->GetLoadedLevel()->Actors)
			{
				if (auto&& TActor = Cast<T>(Actor))
					OutActors.Add(TActor);
			}
		}
	}

	template <class T>
	static void CollectActorInStreamingLvl(const ULevelStreaming* Level, TArray<T*>& OutActors)
	{
		if (Level == nullptr || !Level->IsLevelLoaded())
		{
			return;
		}


		for (auto&& Actor : Level->GetLoadedLevel()->Actors)
		{
			if (auto&& TActor = Cast<T>(Actor))
				OutActors.Add(TActor);
		}
	}
};
