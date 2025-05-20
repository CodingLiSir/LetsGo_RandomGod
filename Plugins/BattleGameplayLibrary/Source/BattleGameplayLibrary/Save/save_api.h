// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "save_interface.h"
#include "sys_save.h"
#include "UObject/Object.h"

enum class ESavePhase :uint8
{
	Invalid,
	Load,
	Save,
	Delete,
	Block,
};

class BATTLEGAMEPLAYLIBRARY_API SaveModuleApi
{
public:
	static ESavePhase GetPhase(const FString& SlotName);

	static void InitModule(const TFunction<USaveGame*(USaveGame* InSaveGame, const FString& SlotName)>& InFunc,
	                       ISaveGameSystem* InSys);
	static void DestroyModule();

	static void RefreshStorage(const FString& SlotPath);
	static void RefreshStorages(const TArray<FString>& SlotPaths);

	static bool Exist(const FString& SlotName);
	static void Filter(TArray<IWashSaveInterface*>& OutSlotNames,
	                   const TFunctionRef<bool(const FString& SlotName, IWashSaveInterface* InSaveGame)> InFilterFunc);

	template <typename T, typename =typename TEnableIf<TIsCastable<T>::Value>::type>
	static void Filter(const FString& DesiredSlotName, TArray<T*>& OutSlotNames)
	{
		TArray<IWashSaveInterface*> slot_save_games_arr;
		SaveModuleApi::Filter(slot_save_games_arr,
		                      [DesiredSlotName](const FString& SlotName, IWashSaveInterface* InSaveGame)-> bool
		                      {
			                      T* saved_uobject = Cast<T>(InSaveGame);
			                      if (saved_uobject && (DesiredSlotName.IsEmpty() || DesiredSlotName == SlotName))
			                      {
				                      return true;
			                      }
			                      return false;
		                      });

		OutSlotNames.Reset();
		for (auto&& save_interface : slot_save_games_arr)
		{
			auto&& save_game = Cast<T>(save_interface);
			if (save_game)
			{
				OutSlotNames.Emplace(save_game);
			}
		}
	}

	static TArray<FString> GetAllSavePath();
	static TArray<IWashSaveInterface*> GetAllSaveGame();

	static bool ValidateSaveData(const FString& SlotName, TArray<uint8>& InBytes, UClass* InClass);
	static bool ValidateSaveData(IWashSaveInterface* InSaveGame);

	static FString GetSaveGamePathConf(const UClass* InCls);
	static UClass* GetSaveGameClassConf(const FString& SlotName);
	static IWashSaveInterface* CreateSync(const FString& SlotName, const UClass* Cls);
	static IWashSaveInterface* CreateSync(const FString& SlotName);
	static IWashSaveInterface* CreateSync(const UClass* Cls);

	template <typename T, typename =typename TEnableIf<TIsCastable<T>::Value>::type>
	static T* CreateSync()
	{
		return Cast<T>(CreateSync(T::StaticClass()));
	}

	template <typename T, typename =typename TEnableIf<TIsCastable<T>::Value>::type>
	static T* CreateSync(const FString& SlotName)
	{
		return Cast<T>(CreateSync(SlotName, T::StaticClass()));
	}

	static int32 SaveSync(IWashSaveInterface* InSaveGame);
	static int32 SaveSync(const FString& SlotName, TArray<uint8>& InBytes, UClass* InClass);
	static int32 SaveAsync(const FString& SlotName, TArray<uint8>& InBytes, UClass* InClass,
	                       FWashOpDelegate OnComplete = {});
	static int32 SaveAsync(IWashSaveInterface* InSaveGame, FWashOpDelegate OnComplete = {});


	template <typename T, typename =typename TEnableIf<TIsCastable<T>::Value>::type>
	static T* LoadSync()
	{
		return Cast<T>(LoadSync(GetSaveGamePathConf(T::StaticClass())));
	}

	template <typename T, typename =typename TEnableIf<TIsCastable<T>::Value>::type>
	static T* LoadSync(const FString& SlotName)
	{
		return Cast<T>(LoadSync(SlotName));
	}

	static IWashSaveInterface* LoadSync(const FString& SlotName);
	static int32 LoadAsync(const FString& SlotName, FWashLoadDelegate OnComplete = {});

	static int32 DeleteSync(const FString& SlotName);
	static int32 DeleteSync(IWashSaveInterface* InSaveGame);
	static int32 DeleteAsync(const FString& SlotName, FWashOpDelegate OnComplete = {});
	static int32 DeleteAsync(IWashSaveInterface* InSaveGame, FWashOpDelegate OnComplete = {});
};
