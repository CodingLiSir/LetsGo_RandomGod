// Fill out your copyright notice in the Description page of Project Settings.


#include "save_api.h"

#include "BattleGamePlayLibrary/BattleLog/BattleLogSave.h"
#include "Kismet/GameplayStatics.h"

ESavePhase SaveModuleApi::GetPhase(const FString& SlotName)
{
	if (SlotName.IsEmpty())
	{
		ERROR_SAVE("SlotName is Empty");
		return ESavePhase::Invalid;
	}

	auto&& save_inst = UWashSaveSystem::Get();
	auto&& save_sys_phase = save_inst->GetPhase(SlotName);

	auto is_blocking = save_inst->_HasPhase(save_sys_phase, ESaveRequestPhase::WriteBlock);
	if (is_blocking)
	{
		return ESavePhase::Block;
	}

	auto is_loading = save_inst->_HasPhase(save_sys_phase, ESaveRequestPhase::Load);
	if (is_loading)
	{
		return ESavePhase::Load;
	}

	auto is_saving = save_inst->_HasPhase(save_sys_phase, ESaveRequestPhase::Save);
	if (is_saving)
	{
		return ESavePhase::Save;
	}

	auto is_deleting = save_inst->_HasPhase(save_sys_phase, ESaveRequestPhase::Delete);
	if (is_deleting)
	{
		return ESavePhase::Delete;
	}

	return ESavePhase::Invalid;
}

void SaveModuleApi::InitModule(const TFunction<USaveGame*(USaveGame* InSaveGame, const FString& SlotName)>& InFunc,
	ISaveGameSystem* InSys)
{
	auto&& save_inst = UWashSaveSystem::Get();
	if (save_inst == nullptr)
	{
		ERROR_SAVE("save_inst is nullptr");
		return;
	}
	save_inst->Init(InFunc, InSys);
}

void SaveModuleApi::DestroyModule()
{
	auto&& save_inst = UWashSaveSystem::Get();
	if (save_inst == nullptr)
	{
		return;
	}
	save_inst->SystemPhase = ESaveSystemPhase::Invalid;
}

void SaveModuleApi::RefreshStorage(const FString& SlotPath)
{
	if (SlotPath.IsEmpty())
	{
		ERROR_SAVE("SlotName is Empty");
		return;
	}

	SaveModuleApi::LoadSync(SlotPath);
}

void SaveModuleApi::RefreshStorages(const TArray<FString>& SlotPaths)
{
	for (auto&& Element : SlotPaths)
	{
		RefreshStorage(Element);
	}
}

bool SaveModuleApi::Exist(const FString& SlotName)
{
	if (SlotName.IsEmpty())
	{
		ERROR_SAVE("SlotName is Empty");
		return false;
	}

	auto&& save_inst = UWashSaveSystem::Get();
	if (save_inst == nullptr)
	{
		return false;
	}
	return save_inst->Exist(SlotName);
}

void SaveModuleApi::Filter(TArray<IWashSaveInterface*>& OutSlotNames,
                           const TFunctionRef<bool(const FString& SlotName, IWashSaveInterface* InSaveGame)>
                           InFilterFunc)
{
	OutSlotNames.Reset();

	auto&& save_inst = UWashSaveSystem::Get();
	if (save_inst == nullptr)
	{
		return;
	}
	save_inst->Filter(InFilterFunc, OutSlotNames);
}

TArray<FString> SaveModuleApi::GetAllSavePath()
{
	TArray<FString> all_path_arr;

	auto&& save_inst = UWashSaveSystem::Get();
	for (auto&& [slot_name,save_index] : save_inst->Storage_SaveIndexes)
	{
		if (save_inst->Storage_SaveGames.IsValidIndex(save_index) == false)
		{
			continue;
		}

		auto&& weak_ptr = save_inst->Storage_SaveGames[save_index];
		if (weak_ptr.IsValid() == false)
		{
			continue;
		}

		all_path_arr.Emplace(slot_name);
	}

	return all_path_arr;
}

TArray<IWashSaveInterface*> SaveModuleApi::GetAllSaveGame()
{
	TArray<IWashSaveInterface*> all_save_games;

	auto&& save_inst = UWashSaveSystem::Get();
	for (auto&& [slot_name,save_index] : save_inst->Storage_SaveIndexes)
	{
		if (save_inst->Storage_SaveGames.IsValidIndex(save_index) == false)
		{
			continue;
		}

		auto&& weak_ptr = save_inst->Storage_SaveGames[save_index];
		if (weak_ptr.IsValid() == false)
		{
			continue;
		}

		IWashSaveInterface* save_game_interface = Cast<IWashSaveInterface>(weak_ptr.Get());
		if (save_game_interface == nullptr)
		{
			continue;
		}

		all_save_games.Emplace(save_game_interface);
	}

	return all_save_games;
}

bool SaveModuleApi::ValidateSaveData(const FString& SlotName, TArray<uint8>& InBytes, UClass* InClass)
{
	if (SlotName.IsEmpty())
	{
		ERROR_SAVE("SlotName is Empty");
		return false;
	}

	auto&& save_inst = UWashSaveSystem::Get();
	return save_inst->ValidateSaveData(SlotName, InBytes, InClass);
}

bool SaveModuleApi::ValidateSaveData(IWashSaveInterface* InSaveGame)
{
	if (InSaveGame == nullptr)
	{
		ERROR_SAVE("InSaveGame is nullptr");
		return false;
	}

	auto&& save_inst = UWashSaveSystem::Get();
	return save_inst->ValidateSaveData(InSaveGame);
}

FString SaveModuleApi::GetSaveGamePathConf(const UClass* InCls)
{
	auto&& save_inst = UWashSaveSystem::Get();
	return save_inst->_GetSaveGamePathConf(InCls);
}

UClass* SaveModuleApi::GetSaveGameClassConf(const FString& SlotName)
{
	if (SlotName.IsEmpty())
	{
		ERROR_SAVE("SlotName is Empty");
		return nullptr;
	}
	auto&& save_inst = UWashSaveSystem::Get();
	return save_inst->_GetSaveGameClass(SlotName);
}

IWashSaveInterface* SaveModuleApi::CreateSync(const FString& SlotName, const UClass* save_cls)
{
	if (SlotName.IsEmpty())
	{
		ERROR_SAVE("SlotName is Empty");
		return nullptr;
	}

	if (IsValid(save_cls) == false)
	{
		WARN_SAVE("GetSaveGameClass is nullptr SlotName : %s", *SlotName);
		return nullptr;
	}

	auto&& save_game_interface = Cast<IWashSaveInterface>(NewObject<USaveGame>(GetTransientPackage(), save_cls));
	if (save_game_interface == nullptr)
	{
		WARN_SAVE("CreateSaveGameObject is nullptr SlotName : %s", *SlotName);
		return nullptr;
	}

	save_game_interface->SetSlotName(SlotName);

	if (ValidateSaveData(save_game_interface) == false)
	{
		WARN_SAVE("ValidateSaveData failed");
		return nullptr;
	}
	auto&& save_inst = UWashSaveSystem::Get();
	//2.同步保存，一旦在删除的则移除
	auto&& slot_save_phase = save_inst->GetPhase(SlotName);
	bool is_blocking = save_inst->_HasPhase(slot_save_phase, ESaveRequestPhase::WriteBlock);

	//阻塞的话，则暂停
	if (is_blocking)
	{
		ERROR_SAVE("%s is Blocking", *save_game_interface->GetSlotName());
		return nullptr;
	}
	//3.保存
	int32 is_suc = save_inst->SaveSync(save_game_interface) ;
	if (is_suc == 0)
	{
		return save_game_interface;
	}

	return nullptr;
}

IWashSaveInterface* SaveModuleApi::CreateSync(const FString& SlotName)
{
	if (SlotName.IsEmpty())
	{
		ERROR_SAVE("SlotName is Empty");
		return nullptr;
	}

	auto&& save_inst = UWashSaveSystem::Get();

	auto save_cls = SaveModuleApi::GetSaveGameClassConf(SlotName);
	if (IsValid(save_cls) == false)
	{
		ERROR_SAVE("GetSaveGameClass is nullptr SlotName : %s", *SlotName);
		return nullptr;
	}

	auto&& save_game_interface = Cast<IWashSaveInterface>(NewObject<USaveGame>(GetTransientPackage(), save_cls));
	if (save_game_interface == nullptr)
	{
		WARN_SAVE("CreateSaveGameObject is nullptr SlotName : %s", *SlotName);
		return nullptr;
	}

	if (ValidateSaveData(save_game_interface) == false)
	{
		WARN_SAVE("ValidateSaveData failed");
		return nullptr;
	}

	//2.同步保存，一旦在删除的则移除
	auto&& slot_save_phase = save_inst->GetPhase(SlotName);

	bool is_blocking = save_inst->_HasPhase(slot_save_phase, ESaveRequestPhase::WriteBlock);

	//阻塞的话，则暂停
	if (is_blocking)
	{
		ERROR_SAVE("%s is Blocking", *SlotName);
		return nullptr;
	}

	//3.保存
	save_game_interface->SetSlotName(SlotName);
	int32 is_suc = save_inst->SaveSync(save_game_interface);
	if (is_suc == 0)
	{
		return save_game_interface;
	}

	return nullptr;
}

IWashSaveInterface* SaveModuleApi::CreateSync(const UClass* save_cls)
{
	// UWorld* world = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
	// if (world == nullptr)
	// {
	// 	WARN_SAVE("WorldContext is nullptr");
	// 	return nullptr;
	// }

	if (IsValid(save_cls) == false)
	{
		ERROR_SAVE("GetSaveGameClass is nullptr");
		return nullptr;
	}

	auto slot_name = GetSaveGamePathConf(save_cls);
	if (slot_name.IsEmpty())
	{
		WARN_SAVE("GetSaveGamePathConf is Empty");
		return nullptr;
	}

	auto&& save_game_interface = Cast<IWashSaveInterface>(NewObject<USaveGame>(GetTransientPackage(), save_cls));
	if (save_game_interface == nullptr)
	{
		WARN_SAVE("CreateSaveGameObject is nullptr ");
		return nullptr;
	}

	if (ValidateSaveData(save_game_interface) == false)
	{
		WARN_SAVE("ValidateSaveData failed");
		return nullptr;
	}

	auto&& save_inst = UWashSaveSystem::Get();
	//2.同步保存，一旦在删除的则移除
	auto&& slot_save_phase = save_inst->GetPhase(slot_name);
	bool is_blocking = save_inst->_HasPhase(slot_save_phase, ESaveRequestPhase::WriteBlock);

	//阻塞的话，则暂停
	if (is_blocking)
	{
		ERROR_SAVE("%s is Blocking", *save_game_interface->GetSlotName());
		return nullptr;
	}
	//3.保存
	int32 is_suc = save_inst->SaveSync(save_game_interface); 
	if (is_suc == 0)
	{
		return save_game_interface;
	}

	return nullptr;
}

int32 SaveModuleApi::SaveSync(IWashSaveInterface* InSaveGame)
{
	if (InSaveGame == nullptr)
	{
		ERROR_SAVE("InSaveGame is nullptr");
		return -1;
	}

	auto&& in_slot_name = InSaveGame->GetSlotName();
	if (in_slot_name.IsEmpty())
	{
		ERROR_SAVE("SlotName is Empty");
		return -1;
	}

	auto&& save_inst = UWashSaveSystem::Get();
	return save_inst->SaveSync(InSaveGame);
}

int32 SaveModuleApi::SaveSync(const FString& SlotName, TArray<uint8>& InBytes, UClass* InClass)
{
	if (InClass == nullptr)
	{
		ERROR_SAVE("InClass is nullptr");
		return -1;
	}

	if (SlotName.IsEmpty())
	{
		ERROR_SAVE("SlotName is Empty");
		return -1;
	}

	if (InBytes.Num() == 0)
	{
		ERROR_SAVE("InBytes is Empty");
		return -1;
	}

	auto&& save_inst = UWashSaveSystem::Get();
	return save_inst->SaveSync(SlotName, InBytes, InClass);
}

int32 SaveModuleApi::SaveAsync(const FString& SlotName, TArray<uint8>& InBytes, UClass* InClass,
                               FWashOpDelegate OnComplete)
{
	if (InClass == nullptr)
	{
		ERROR_SAVE("InClass is nullptr");
		return -1;
	}

	if (SlotName.IsEmpty())
	{
		ERROR_SAVE("SlotName is Empty");
		return -1;
	}

	if (InBytes.Num() == 0)
	{
		ERROR_SAVE("InBytes is Empty");
		return -1;
	}

	auto&& save_inst = UWashSaveSystem::Get();
	return save_inst->SaveAsync(SlotName, InBytes, InClass, OnComplete);
}

int32 SaveModuleApi::SaveAsync(IWashSaveInterface* InSaveGame, FWashOpDelegate OnComplete)
{
	if (InSaveGame == nullptr)
	{
		ERROR_SAVE("InSaveGame is nullptr");
		return -1;
	}

	auto&& in_slot_name = InSaveGame->GetSlotName();
	if (in_slot_name.IsEmpty())
	{
		ERROR_SAVE("SlotName is Empty");
		return -1;
	}

	auto&& save_inst = UWashSaveSystem::Get();
	return save_inst->SaveAsync(in_slot_name, InSaveGame, OnComplete);
}

IWashSaveInterface* SaveModuleApi::LoadSync(const FString& SlotName)
{
	if (SlotName.IsEmpty())
	{
		ERROR_SAVE("SlotName is Empty");
		return nullptr;
	}

	IWashSaveInterface* save_game = nullptr;

	auto&& save_inst = UWashSaveSystem::Get();
	auto ret_code = save_inst->LoadSync(SlotName, save_game);
	if (ret_code == 0)
	{
		return save_game;
	}
	return nullptr;
}

int32 SaveModuleApi::LoadAsync(const FString& SlotName, FWashLoadDelegate OnComplete)
{
	if (SlotName.IsEmpty())
	{
		ERROR_SAVE("SlotName is Empty");
		return -1;
	}

	auto&& save_inst = UWashSaveSystem::Get();
	return save_inst->LoadAsync(SlotName, OnComplete);
}

int32 SaveModuleApi::DeleteSync(const FString& SlotName)
{
	if (SlotName.IsEmpty())
	{
		ERROR_SAVE("SlotName is Empty");
		return -1;
	}

	auto&& save_inst = UWashSaveSystem::Get();
	return save_inst->DeleteSync(SlotName);
}

int32 SaveModuleApi::DeleteSync(IWashSaveInterface* InSaveGame)
{
	if (InSaveGame == nullptr)
	{
		ERROR_SAVE("InSaveGame is nullptr");
		return -1;
	}

	auto&& save_inst = UWashSaveSystem::Get();
	return save_inst->DeleteSync(InSaveGame);
}

int32 SaveModuleApi::DeleteAsync(const FString& SlotName, FWashOpDelegate OnComplete)
{
	if (SlotName.IsEmpty())
	{
		ERROR_SAVE("SlotName is Empty");
		return -1;
	}

	auto&& save_inst = UWashSaveSystem::Get();
	return save_inst->DeleteAsync(SlotName, OnComplete);
}

int32 SaveModuleApi::DeleteAsync(IWashSaveInterface* InSaveGame, FWashOpDelegate OnComplete)
{
	if (InSaveGame == nullptr)
	{
		ERROR_SAVE("InSaveGame is nullptr");
		return -1;
	}

	auto&& save_inst = UWashSaveSystem::Get();
	return save_inst->DeleteAsync(InSaveGame, OnComplete);
}
