// Fill out your copyright notice in the Description page of Project Settings.


#include "sys_save.h"

#include "PlatformFeatures.h"
#include "SaveGameSystem.h"
#include "savegame_bytes.h"
#include "save_api.h"
#include "save_interface.h"
#include "save_request.h"
#include "BattleGamePlayLibrary/BattleLog/BattleLogSave.h"
#include "BattleGamePlayLibrary/Utility/BattleLibNativeUtil.h"
#include "Kismet/GameplayStatics.h"

//using namespace Next::magic_enum::bitwise_operators;

UE::Tasks::FPipe UWashSaveSystem::AsyncTaskPipe{TEXT("WashSaveGamePipe")};


//多线程的回调
void _OnSaveGameToSlotCompleted(const FString& InSlotName, FPlatformUserId InUserIndex, bool bDataSaved)
{
	// LOG_SAVE("SaveGameToSlot Completed :%s  bDataSaved:%d", *InSlotName, bDataSaved);

	auto save_inst = UWashSaveSystem::Get();
	if (save_inst->SystemPhase != ESaveSystemPhase::Initilized)
	{
		return;
	}

	save_inst->_UpdateRequest(InSlotName, ESaveRequestPhase::Save, [bDataSaved](FSaveGameRequest& request)
	{
		request.IsFinished = true;
		request.IsSuccess = bDataSaved;
	});
}

//多线程的回调
void _OnDeleteGameToSlotCompleted(const FString& InSlotName, FPlatformUserId InUserIndex, bool bDataSaved)
{
	// LOG_SAVE("DeleteGameToSlot Completed :%s  bDataSaved:%d", *InSlotName, bDataSaved);

	auto save_inst = UWashSaveSystem::Get();
	if (save_inst->SystemPhase != ESaveSystemPhase::Initilized)
	{
		return;
	}
	save_inst->_UpdateRequest(InSlotName, ESaveRequestPhase::Delete, [bDataSaved](FSaveGameRequest& request)
	{
		request.IsFinished = true;
		request.IsSuccess = bDataSaved;
	});
}

//多线程的回调
//const FString&, FPlatformUserId, bool, const TArray<uint8>&
void _OnLoadGameFromSlotCompleted(const FString& InSlotName, FPlatformUserId InUserIndex, bool bDataSaved,
                                  const TArray<uint8>& InBytes)
{
	// LOG_SAVE("LoadGameFromSlot Completed :%s  bDataSaved:%d", *InSlotName, bDataSaved);

	auto save_inst = UWashSaveSystem::Get();
	if (save_inst->SystemPhase != ESaveSystemPhase::Initilized)
	{
		return;
	}
	save_inst->_UpdateRequest(InSlotName, ESaveRequestPhase::Load, [&InBytes,bDataSaved](FSaveGameRequest& request)
	{
		request.Bytes.Append(InBytes);
		request.IsFinished = true;
		request.IsSuccess = bDataSaved;
	});
}

ISaveGameSystem* UWashSaveSystem::GetPlatformSaveSystem()
{
	if (UWashSaveSystem::PlatFormSaveSystem == nullptr)
	{
		UWashSaveSystem::PlatFormSaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem();
	}

	return UWashSaveSystem::PlatFormSaveSystem;
}

UWashSaveSystem* UWashSaveSystem::Get()
{
	static UWashSaveSystem* Inst;
	if (Inst == nullptr && GEngine != nullptr)
	{
		Inst = GEngine->GetEngineSubsystem<UWashSaveSystem>();
	}
	return Inst;
}

void UWashSaveSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	//注册相关回调
	OnSaveGameDelegate = _OnSaveGameToSlotCompleted;
	OnDeleteGameDelegate = _OnDeleteGameToSlotCompleted;
	OnLoadGameDelegate = _OnLoadGameFromSlotCompleted;
}

void UWashSaveSystem::Deinitialize()
{
	_CleanGameData();
	FWorldDelegates::OnPreWorldInitialization.RemoveAll(this);

	Super::Deinitialize();
}

void UWashSaveSystem::Init(const TFunction<USaveGame*(USaveGame* InSaveGame, const FString& SlotName)>& InFunc,
                           ISaveGameSystem* InSys)
{
	ProcessFunction = InFunc;

	if (PlatFormSaveSystem != InSys)
	{
		PlatFormSaveSystem = InSys;

		_CleanGameData();
		_InitSystem();
	}
}

void UWashSaveSystem::_CleanGameData()
{
	SystemPhase = ESaveSystemPhase::Invalid;
	SavingTasks.Empty();
	DeletingTasks.Empty();
	LoadingTasks.Empty();
	AvailableClasses.Empty();
	SaveConfList.Empty();
	Storage_SaveIndexes.Empty();
	Storage_SaveGames.Empty();
	Storage_Persistence_SaveGames.Empty();
}

void UWashSaveSystem::_InitSystem()
{
	AvailableClasses.Reset();

	TArray<UClass*> all_cls_array;
	GetDerivedClasses(USaveGame::StaticClass(), all_cls_array);

	AvailableClasses.Append(all_cls_array);

	SaveConfList.Reset();
	for (auto cls : AvailableClasses)
	{
		IWashSaveInterface* save_game = Cast<IWashSaveInterface>(cls->GetDefaultObject());
		if (save_game)
		{
			FWashSaveConf conf;
			conf.SaveUserCls = cls;
			conf.SaveSlotName = save_game->GetSlotName();
			SaveConfList.Emplace(conf);
		}
	}

	SystemPhase = ESaveSystemPhase::Initilized;

	ScanSaveGames();

	OnLoadComplete.Broadcast();
}

void UWashSaveSystem::ScanSaveGames()
{
	TArray<FString> save_game_files;
	if (GetPlatformSaveSystem()->GetSaveGameNames(save_game_files, 0) == false)
	{
		return;
	}

	//全部的文件列表
	TSet<FString> all_files_set{save_game_files};

	//移除所有不存在的
	for (auto&& [save_file_path,save_index] : Storage_SaveIndexes)
	{
		//在当前的文件列表中，但是不在新的文件列表中，就是要移除的
		if (all_files_set.Contains(save_file_path))
		{
			continue;
		}
		_RemoveStorage(save_file_path);

		//从所有的队列中删除它
		_ClearTask(save_file_path, ESaveRequestPhase::Save);
		_ClearTask(save_file_path, ESaveRequestPhase::Load);
	}

	//添加所有存在的
	for (auto&& save_file_path : save_game_files)
	{
		//如果不在当前的文件列表中，就是新的
		if (Storage_SaveIndexes.Contains(save_file_path) == false)
		{
			IWashSaveInterface* save_game = nullptr;

			TArray<uint8> Bytes;

			auto load_suc = GetPlatformSaveSystem()->LoadGame(false, *save_file_path, 0, Bytes);
			if (load_suc == false)
			{
				continue;
			}
			USaveGame* save_game_loaded = UGameplayStatics::LoadGameFromMemory(Bytes);
			if (UWashSaveSystem::ProcessFunction != nullptr)
			{
				save_game = Cast<
					IWashSaveInterface>(UWashSaveSystem::ProcessFunction(save_game_loaded, save_file_path));
			}


			if (save_game != nullptr)
			{
				save_game->SetSlotName(save_file_path);
				_AddOrRefreshStorage(save_game);
			}
			else
			{
				WARN_SAVE("save_game :%s is null from filestorge", *save_file_path);
			}
		}
	}

	OnRefreshComplete.Broadcast();
}

bool UWashSaveSystem::IsValidSave(UClass* InSaveGame)
{
	if (InSaveGame == nullptr)
	{
		return false;
	}
	return AvailableClasses.Contains(InSaveGame);
}

ESaveRequestPhase UWashSaveSystem::GetPhase(const FString& SlotName)
{
	ESaveRequestPhase result = ESaveRequestPhase::Free;

	_IteratorAllRequests([&SlotName,&result](TMap<FString, TDeque<FSaveGameRequest>>& Tasks)
	{
		auto task_queue = Tasks.Find(SlotName);
		if (task_queue == nullptr)
		{
			return;
		}

		if (task_queue->Num() == 0)
		{
			Tasks.Remove(SlotName);
			return;
		}

		auto&& task = task_queue->First();
		if (task.IsFinished == false)
		{
			result |= task.Phase;
		}
	});

	return result;
}

bool UWashSaveSystem::Exist(const FString& SlotName)
{
	if (SystemPhase != ESaveSystemPhase::Initilized)
	{
		WARN_SAVE(" SaveSystem is not Initilized");
		return false;
	}

	return GetPlatformSaveSystem()->DoesSaveGameExist(*SlotName, 0); //Storage_SaveIndexes.Contains(SlotName);
}

void UWashSaveSystem::Filter(
	const TFunctionRef<bool(const FString& SlotName, IWashSaveInterface* InSaveGame)> InFilterFunc,
	TArray<IWashSaveInterface*>& OutSlotNames)
{
	if (SystemPhase != ESaveSystemPhase::Initilized)
	{
		WARN_SAVE(" SaveSystem is not Initilized");
		return;
	}

	for (auto&& [in_slot,slot_index] : Storage_SaveIndexes)
	{
		if (Storage_SaveGames.IsValidIndex(slot_index) == false)
		{
			continue;
		}

		auto&& save_game = Storage_SaveGames[slot_index];
		if (save_game.IsValid() == false)
		{
			continue;
		}

		IWashSaveInterface* save_game_interface = Cast<IWashSaveInterface>(save_game.Get());
		if (save_game_interface == nullptr)
		{
			continue;
		}

		if (InFilterFunc(in_slot, save_game_interface))
		{
			OutSlotNames.Emplace(save_game_interface);
		}
	}
}

bool UWashSaveSystem::ValidateSaveData(IWashSaveInterface* InSaveGame)
{
	if (SystemPhase != ESaveSystemPhase::Initilized)
	{
		WARN_SAVE(" SaveSystem is not Initilized");
		return false;
	}

	//判断类型是否匹配
	auto&& save_game = Cast<UObject>(InSaveGame);
	if (save_game == nullptr)
	{
		WARN_SAVE("InSaveGame is not a uobject");
		return false;
	}

	if (AvailableClasses.Contains(save_game->GetClass()) == false)
	{
		WARN_SAVE("%s is not valid save game, class is %s", *InSaveGame->GetSlotName(),
		          *save_game->GetClass()->GetName());
		return false;
	}

	return true;
}

bool UWashSaveSystem::ValidateSaveData(const FString& SlotName, TArray<uint8>& InBytes, UClass* InClass)
{
	if (SystemPhase != ESaveSystemPhase::Initilized)
	{
		WARN_SAVE(" SaveSystem is not Initilized");
		return false;
	}

	auto save_conf_class = _GetSaveGameClass(SlotName);
	if (save_conf_class && save_conf_class != InClass)
	{
		WARN_SAVE("it is not valid save game, class is %s but conf is %s", *InClass->GetName(),
		          *save_conf_class->GetName());
		return false;
	}

	if (AvailableClasses.Contains(InClass) == false)
	{
		WARN_SAVE("it is not valid save game, class is %s", *InClass->GetName());
		return false;
	}

	return true;
}

int32 UWashSaveSystem::SaveSync(IWashSaveInterface* InSaveGame)
{
	if (SystemPhase != ESaveSystemPhase::Initilized)
	{
		WARN_SAVE(" SaveSystem is not Initilized");
		return -1;
	}

	if (ValidateSaveData(InSaveGame) == false)
	{
		ERROR_SAVE("ValidateSaveData failed");
		return -1;
	}

	//2.同步保存，一旦在删除的则移除
	auto&& slot_save_phase = GetPhase(InSaveGame->GetSlotName());

	bool is_blocking = _HasPhase(slot_save_phase, ESaveRequestPhase::WriteBlock);

	//阻塞的话，则暂停
	if (is_blocking)
	{
		ERROR_SAVE("%s is Blocking", *InSaveGame->GetSlotName());
		return -1;
	}

	//3.保存
	TArray<uint8> mem_bytes;
	UGameplayStatics::SaveGameToMemory(Cast<USaveGame>(InSaveGame), mem_bytes);

	bool is_suc = GetPlatformSaveSystem()->SaveGame(false, *InSaveGame->GetSlotName(), 0, mem_bytes);
	if (is_suc)
	{
		_AddOrRefreshStorage(InSaveGame);
	}

	LOG_SAVE("save sync slot:%s ,result:%s,phase:%s", *InSaveGame->GetSlotName(), is_suc ? TEXT("true") : TEXT("false"),
	         *BattleLibNativeUtil::EnumToFName(slot_save_phase).ToString());
	return is_suc ? 0 : -1;
}

int32 UWashSaveSystem::SaveSync(const FString& SlotName, TArray<uint8>& InBytes, UClass* InClass)
{
	if (SystemPhase != ESaveSystemPhase::Initilized)
	{
		WARN_SAVE(" SaveSystem is not Initilized");
		return -1;
	}

	//1.保存的时候判断数据有效性
	if (ValidateSaveData(SlotName, InBytes, InClass) == false)
	{
		ERROR_SAVE("save data is invalid");
		return -1;
	}

	//2.同步保存，一旦在删除的则移除
	auto&& slot_save_phase = GetPhase(SlotName);

	bool is_blocking = _HasPhase(slot_save_phase, ESaveRequestPhase::WriteBlock);
	// bool is_saving = _HasPhase(slot_save_phase, ESaveRequestPhase::Save);

	//阻塞的话，则暂停
	if (is_blocking)
	{
		ERROR_SAVE("%s is Blocking", *SlotName);
		return -1;
	}
	// else if (is_saving)
	// {
	// 	//3.添加到请求队列
	// 	//保存的时候，需要判定是否内容一致，如果内容一致则不需要保存，如果内容不一致则需要保存
	// 	//考虑到CRC的性能问题，不做检查，有新的请求就添加到队列中
	// 	auto&& save_queue = SavingTasks.FindOrAdd(SlotName);
	// 	FSaveGameRequest request;
	// 	request.SaveSlot = SlotName;
	// 	request.Phase = ESaveRequestPhase::Save;
	// 	request.Bytes->Append(InBytes);
	//
	// 	save_queue.PushLast(request);
	// 	return 1;
	// }

	auto&& save_game = UGameplayStatics::LoadGameFromMemory(InBytes);
	if (UWashSaveSystem::ProcessFunction != nullptr)
	{
		save_game = UWashSaveSystem::ProcessFunction(save_game, SlotName);
	}

	auto&& save_game_interface = Cast<IWashSaveInterface>(save_game);
	if (save_game == nullptr || save_game->GetClass() != InClass || save_game_interface == nullptr)
	{
		ERROR_SAVE("save_game is null or class is not match");
		return -1;
	}

	//3.保存
	bool is_suc = GetPlatformSaveSystem()->SaveGame(false, *SlotName, 0, InBytes);
	if (is_suc)
	{
		_AddOrRefreshStorage(save_game_interface);
	}

	LOG_SAVE("save sync slot:%s ,result:%s,phase:%s", *SlotName, is_suc ? TEXT("true") : TEXT("false"),
	         *BattleLibNativeUtil::EnumToFName(slot_save_phase).ToString());
	return is_suc ? 0 : -1;
}

int32 UWashSaveSystem::SaveAsync(const FString& SlotName, IWashSaveInterface* InSaveGame,
                                 FWashOpDelegate OnComplete)
{
	if (SystemPhase != ESaveSystemPhase::Initilized)
	{
		WARN_SAVE(" SaveSystem is not Initilized");
		return -1;
	}

	//1.保存的时候判断数据有效性
	if (ValidateSaveData(InSaveGame) == false)
	{
		ERROR_SAVE("save data is invalid");
		return -1;
	}
	// TArray<uint8> mem_bytes;
	// UGameplayStatics::SaveGameToMemory(Cast<USaveGame>(InSaveGame), mem_bytes);

	//2.确定保存的阶段
	//拿到任务的阶段
	auto&& slot_save_phase = GetPhase(SlotName);

	bool is_deleting = _HasPhase(slot_save_phase, ESaveRequestPhase::Delete);
	bool is_blocking = _HasPhase(slot_save_phase, ESaveRequestPhase::WriteBlock);

	if (is_deleting || is_blocking)
	{
		ERROR_SAVE("%s is deleting :%d or blocking :%d", *SlotName, is_deleting, is_blocking);
		return -1;
	}

	//3.添加到请求队列
	//保存的时候，需要判定是否内容一致，如果内容一致则不需要保存，如果内容不一致则需要保存
	//考虑到CRC的性能问题，不做检查，有新的请求就添加到队列中
	auto&& save_queue = SavingTasks.FindOrAdd(SlotName);

	FSaveGameRequest request;
	request.SaveSlot = SlotName;
	request.Phase = ESaveRequestPhase::Save;
	request.SaveGame = Cast<USaveGame>(InSaveGame);
#if !WITH_EDITOR
	if (GWorld)
	{
		request.CreateTimeStamp = GWorld->TimeSeconds;
	}
#endif
	request.OnCompleteDelegate = [InSaveGame,OnComplete](FSaveGameRequest& in_request)
	{
		OnComplete.Broadcast(in_request.SaveSlot, in_request.IsSuccess);
	};

	save_queue.PushLast(request);

	LOG_SAVE("save async %s current queue size %d", *SlotName, save_queue.Num());
	return 0;
}

int32 UWashSaveSystem::SaveAsync(const FString& SlotName, TArray<uint8>& InBytes, UClass* InClass,
                                 FWashOpDelegate OnComplete)
{
	if (SystemPhase != ESaveSystemPhase::Initilized)
	{
		WARN_SAVE(" SaveSystem is not Initilized");
		return -1;
	}

	//1.保存的时候判断数据有效性
	if (ValidateSaveData(SlotName, InBytes, InClass) == false)
	{
		ERROR_SAVE("save data is invalid");
		return -1;
	}

	//2.确定保存的阶段
	// ESaveRequestPhase save_request_phase = ESaveRequestPhase::Save;
	//
	// //拿到任务的阶段
	// auto&& slot_save_phase = GetPhase(SlotName);
	//
	// bool is_deleting = _Is(slot_save_phase, ESaveRequestPhase::Delete);
	// bool is_blocking = _Is(slot_save_phase, ESaveRequestPhase::Block);
	// bool is_saving = _Is(slot_save_phase, ESaveRequestPhase::Save);
	//
	// if (is_deleting && !is_blocking)
	// {
	// 	ERROR_SAVE("%s is deleting", *SlotName);
	// 	return -1;
	// }

	//3.添加到请求队列
	//保存的时候，需要判定是否内容一致，如果内容一致则不需要保存，如果内容不一致则需要保存
	//考虑到CRC的性能问题，不做检查，有新的请求就添加到队列中
	auto&& save_queue = SavingTasks.FindOrAdd(SlotName);

	FSaveGameRequest request;
	request.SaveSlot = SlotName;
	request.Phase = ESaveRequestPhase::Save;
#if !WITH_EDITOR
	if (GWorld)
	{
		request.CreateTimeStamp = GWorld->TimeSeconds;
	}
#endif
	request.Bytes.Append(InBytes);
	request.OnCompleteDelegate = [OnComplete](FSaveGameRequest& in_request)
	{
		OnComplete.Broadcast(in_request.SaveSlot, in_request.IsSuccess);
	};

	save_queue.PushLast(request);

	LOG_SAVE("save async %s current queue size %d", *SlotName, save_queue.Num());
	return 0;
}

int32 UWashSaveSystem::LoadSync(const FString& SlotName, IWashSaveInterface*& OutSaveGame)
{
	if (SystemPhase != ESaveSystemPhase::Initilized)
	{
		WARN_SAVE(" SaveSystem is not Initilized");
		return -1;
	}

	//1。判断是否有这个文件
	if (Exist(SlotName) == false)
	{
		// WARN_SAVE("%s is not exist", *SlotName);
		return -1;
	}
	//2.判断是否blocking
	auto&& slot_save_phase = GetPhase(SlotName);
	bool is_blocking = _HasPhase(slot_save_phase, ESaveRequestPhase::WriteBlock);
	if (is_blocking)
	{
		ERROR_SAVE("%s is blocking", *SlotName);
		return -1;
	}
	//3.进行同步加载解析，有缓存则直接返回，没有则进行加载
	auto&& save_data = _FindSave(SlotName);
	if (save_data == nullptr)
	{
		// ERROR_SAVE("%s is not exist", *SlotName);
		TArray<uint8> Bytes;

		auto load_suc = GetPlatformSaveSystem()->LoadGame(false, *SlotName, 0, Bytes);
		if (load_suc)
		{
			USaveGame* save_game_loaded = UGameplayStatics::LoadGameFromMemory(Bytes);
			if (UWashSaveSystem::ProcessFunction != nullptr)
			{
				save_data = Cast<IWashSaveInterface>(UWashSaveSystem::ProcessFunction(save_game_loaded, SlotName));
			}

			//刷新缓存
			_AddOrRefreshStorage(save_data);
		}
	}

	if (save_data == nullptr)
	{
		ERROR_SAVE("load_game failed %s ", *SlotName);
		return -1;
	}

	OutSaveGame = save_data;

	// LOG_SAVE("load sync %s", *SlotName);
	return 0;
}

int32 UWashSaveSystem::LoadAsync(const FString& SlotName, FWashLoadDelegate OnComplete)
{
	if (SystemPhase != ESaveSystemPhase::Initilized)
	{
		WARN_SAVE(" SaveSystem is not Initilized");
		return -1;
	}
	//1。判断是否有这个文件
	if (Exist(SlotName) == false)
	{
		WARN_SAVE("%s is not exist", *SlotName);
		return -1;
	}

	auto&& save_data = _FindSave(SlotName);
	if (save_data)
	{
		TScriptInterface<IWashSaveInterface> save_game_interface;
		if (save_data)
		{
			save_game_interface = save_data->_getUObject();
		}
		OnComplete.Broadcast(SlotName, save_game_interface, true);
		LOG_SAVE("load async %s", *SlotName);
		return 0;
	}

	//2.加入到load队列中
	auto&& save_queue = LoadingTasks.FindOrAdd(SlotName);

	FSaveGameRequest request;
	request.SaveSlot = SlotName;
	request.Phase = ESaveRequestPhase::Load;
#if !WITH_EDITOR
	if (GWorld)
	{
		request.CreateTimeStamp = GWorld->TimeSeconds;
	}
#endif
	request.OnCompleteDelegate = [OnComplete](FSaveGameRequest& in_request)
	{
		auto save_game = UWashSaveSystem::Get()->_FindSave(in_request.SaveSlot);

		TScriptInterface<IWashSaveInterface> save_game_interface;
		if (save_game)
		{
			save_game_interface.SetObject(save_game->_getUObject());
		}

		OnComplete.Broadcast(in_request.SaveSlot, save_game_interface, in_request.IsSuccess);
	};

	save_queue.PushLast(request);

	return 0;
}

int32 UWashSaveSystem::DeleteSync(const FString& SlotName)
{
	if (SystemPhase != ESaveSystemPhase::Initilized)
	{
		WARN_SAVE(" SaveSystem is not Initilized");
		return -1;
	}

	//1。判断是否有这个文件
	if (Exist(SlotName) == false)
	{
		WARN_SAVE("%s is not exist", *SlotName);
		return -1;
	}

	//2.判断是否blocking
	auto&& slot_save_phase = GetPhase(SlotName);
	bool is_blocking = _HasPhase(slot_save_phase, ESaveRequestPhase::WriteBlock);
	if (is_blocking)
	{
		ERROR_SAVE("%s is blocking", *SlotName);
		return -1;
	}

	//3.执行删除
	if (GetPlatformSaveSystem()->DeleteGame(false, *SlotName, 0))
	{
		_RemoveStorage(SlotName);
		LOG_SAVE("delete %s success", *SlotName);
		return 0;
	}
	ERROR_SAVE("delete %s failed", *SlotName);
	return -1;
}

int32 UWashSaveSystem::DeleteSync(IWashSaveInterface* InSaveGame)
{
	if (SystemPhase != ESaveSystemPhase::Initilized)
	{
		WARN_SAVE(" SaveSystem is not Initilized");
		return -1;
	}
	//1。校验数据有效性
	if (ValidateSaveData(InSaveGame) == false)
	{
		ERROR_SAVE("save data is invalid");
		return -1;
	}

	//2、尝试删除

	if (DeleteSync(InSaveGame->GetSlotName()))
	{
		LOG_SAVE("delete %s success", *InSaveGame->GetSlotName());
		return 0;
	}
	LOG_SAVE("delete %s failed", *InSaveGame->GetSlotName());
	return -1;
}


int32 UWashSaveSystem::DeleteAsync(IWashSaveInterface* InSaveGame, FWashOpDelegate OnComplete)
{
	if (SystemPhase != ESaveSystemPhase::Initilized)
	{
		WARN_SAVE(" SaveSystem is not Initilized");
		return -1;
	}

	//1。校验数据有效性
	if (ValidateSaveData(InSaveGame) == false)
	{
		ERROR_SAVE("save data is invalid");
		return -1;
	}

	//2、尝试删除

	return DeleteAsync(InSaveGame->GetSlotName(), OnComplete);
}

int32 UWashSaveSystem::DeleteAsync(const FString& SlotName, FWashOpDelegate OnComplete)
{
	if (SystemPhase != ESaveSystemPhase::Initilized)
	{
		WARN_SAVE(" SaveSystem is not Initilized");
		return -1;
	}

	//1。判断是否有这个文件
	if (Exist(SlotName) == false)
	{
		WARN_SAVE("%s is not exist", *SlotName);
		return -1;
	}

	auto&& save_queue = DeletingTasks.FindOrAdd(SlotName);

	FSaveGameRequest request;
	request.SaveSlot = SlotName;
	request.Phase = ESaveRequestPhase::Delete;

#if !WITH_EDITOR
	if (GWorld)
	{
		request.CreateTimeStamp = GWorld->TimeSeconds;
	}
#endif

	request.OnCompleteDelegate = [OnComplete](FSaveGameRequest& in_request)
	{
		OnComplete.Broadcast(in_request.SaveSlot, in_request.IsSuccess);
	};

	save_queue.PushLast(request);
	return 0;
}

FString UWashSaveSystem::_GetSaveGamePathConf(const UClass* InCls)
{
	for (auto&& save_data : SaveConfList)
	{
		if (save_data.SaveUserCls == InCls)
		{
			return save_data.SaveSlotName;
		}
	}
	return {};
}

UClass* UWashSaveSystem::_GetSaveGameClass(const FString& SlotName)
{
	for (auto&& save_data : SaveConfList)
	{
		if (save_data.SaveSlotName == SlotName)
		{
			return save_data.SaveUserCls;
		}
	}
	return nullptr;
}

IWashSaveInterface* UWashSaveSystem::_FindSave(int32 Index)
{
	if (!Storage_SaveGames.IsValidIndex(Index))
	{
		return nullptr;
	}

	auto&& ptr = Storage_SaveGames[Index];
	if (ptr.IsValid() == false)
	{
		return nullptr;
	}
	IWashSaveInterface* save_game_interface = Cast<IWashSaveInterface>(ptr.Get());
	if (save_game_interface)
	{
		return save_game_interface;
	}
	return nullptr;
}

IWashSaveInterface* UWashSaveSystem::_FindSave(const FString& SlotName)
{
	auto&& index_ptr = Storage_SaveIndexes.Find(SlotName);
	if (index_ptr == nullptr)
	{
		return nullptr;
	}
	return _FindSave(*index_ptr);
}

void UWashSaveSystem::_RunTasks()
{
	//0.完成任务，构建检测列表

	TArray<FString, TInlineAllocator<4>> slot_list;

	for (auto&& [slot_name,save_tasks] : SavingTasks)
	{
		_FinishTask(ESaveRequestPhase::Save, save_tasks);

		if (save_tasks.Num() > 0)
		{
			slot_list.Emplace(slot_name);
		}
	}

	for (auto&& [slot_name,save_tasks] : DeletingTasks)
	{
		_FinishTask(ESaveRequestPhase::Delete, save_tasks);

		if (save_tasks.Num() > 0)
		{
			slot_list.Emplace(slot_name);
		}
	}

	for (auto&& [slot_name,save_tasks] : LoadingTasks)
	{
		_FinishTask(ESaveRequestPhase::Load, save_tasks);

		if (save_tasks.Num() > 0)
		{
			slot_list.Emplace(slot_name);
		}
	}

	for (auto&& slot_name : slot_list)
	{
		//1.根据时间确定任务的优先级，暂时写在这里
		TArray<ESaveRequestPhase, TInlineAllocator<4>> task_priority_list = {
			ESaveRequestPhase::Save, ESaveRequestPhase::Load, ESaveRequestPhase::Delete
		};

		task_priority_list.Sort([this,slot_name](const ESaveRequestPhase& lft, const ESaveRequestPhase& right)
		{
			double lft_time = 0;
			FSaveGameRequest request;
			if (_FindRequest(slot_name, lft, request))
			{
				lft_time = request.CreateTimeStamp;
			}

			double rft_time = 0;
			if (_FindRequest(slot_name, right, request))
			{
				rft_time = request.CreateTimeStamp;
			}
			return lft_time < rft_time;
		});

		//2.进行任务检查,执行
		for (auto task_phase : task_priority_list)
		{
			FSaveGameRequest request;
			if (_FindRequest(slot_name, task_phase, request))
			{
				LOG_SAVE("run async task %s %s", *slot_name, *BattleLibNativeUtil::EnumToFName(task_phase).ToString());
				_RunTask(request);
				LOG_SAVE("run async task %s %s  finished", *slot_name,
				         *BattleLibNativeUtil::EnumToFName(task_phase).ToString());
			}
		}
	}
}

void UWashSaveSystem::_RunTask(FSaveGameRequest& InRequest)
{
	//拿到当前任务的状态阶段
	auto&& cur_task_phase = GetPhase(InRequest.SaveSlot);

	//如果是阻塞的任务，那么就不执行，等待阻塞结束
	bool is_write_blocking = _HasPhase(cur_task_phase, ESaveRequestPhase::WriteBlock);
	if (is_write_blocking)
	{
		LOG_SAVE("task %s is write blocking", *InRequest.SaveSlot);
		return;
	}
	//是否是加载任务
	bool is_loading = _HasPhase(InRequest, ESaveRequestPhase::Load);
	if (is_loading)
	{
		if (InRequest.IsRunning)
		{
			return;
		}
		InRequest.IsRunning = true;
		InRequest.Phase |= ESaveRequestPhase::ReadBlock;

		FPlatformUserId plat_user_id = FPlatformMisc::GetPlatformUserForUserIndex(0);
		_LoadGameAsync(false, *InRequest.SaveSlot, plat_user_id, OnLoadGameDelegate);
		return;
	}

	//正在读写的任务，不执行
	bool is_read_blocking = _HasPhase(cur_task_phase, ESaveRequestPhase::ReadBlock);
	if (is_read_blocking)
	{
		LOG_SAVE("task %s is read blocking", *InRequest.SaveSlot);
		return;
	}

	//删除的任务先执行
	bool is_deleting = _HasPhase(InRequest, ESaveRequestPhase::Delete);
	if (is_deleting)
	{
		if (InRequest.IsRunning)
		{
			return;
		}
		InRequest.IsRunning = true;
		InRequest.Phase |= ESaveRequestPhase::WriteBlock;

		//有文件才能删除
		if (Exist(InRequest.SaveSlot))
		{
			FPlatformUserId plat_user_id = FPlatformMisc::GetPlatformUserForUserIndex(0);
			_DeleteGameAsync(false, *InRequest.SaveSlot, plat_user_id, OnDeleteGameDelegate);
		}
		else
		{
			InRequest.IsFinished = true;
			InRequest.IsSuccess = false;
		}

		return;
	}

	//保存的任务执行
	bool is_saving = _HasPhase(InRequest, ESaveRequestPhase::Save);
	if (is_saving)
	{
		if (InRequest.IsRunning)
		{
			return;
		}
		InRequest.IsRunning = true;
		InRequest.Phase |= ESaveRequestPhase::WriteBlock;

		LOG_SAVE("start SaveGameAsync %s ", *InRequest.SaveSlot);

		_SaveGameAsync(false, *InRequest.SaveSlot, 0, InRequest, OnSaveGameDelegate);
		return;
	}
}

void UWashSaveSystem::_PostProcessSaveTask(FSaveGameRequest& first_task)
{
	if (first_task.IsSuccess == false)
	{
		return;
	}

	USaveGame* loaded_game = UGameplayStatics::LoadGameFromMemory(first_task.Bytes);
	if (UWashSaveSystem::ProcessFunction != nullptr)
	{
		loaded_game = UWashSaveSystem::ProcessFunction(loaded_game, first_task.SaveSlot);
	}

	//需要把数据进行刷新
	auto&& save_data = _FindSave(first_task.SaveSlot);
	if (save_data)
	{
		save_data->CopyFrom(loaded_game);
		return;
	}

	auto target_cls = _GetSaveGameClass(first_task.SaveSlot);
	if (target_cls == nullptr)
	{
		target_cls = USaveGameBytes::StaticClass();
	}


	if (IsValid(loaded_game) && loaded_game->GetClass() != target_cls)
	{
		first_task.IsSuccess = false;
		//把无效的数据删除
		DeleteAsync(first_task.SaveSlot);

		ERROR_SAVE("loaded game class %s is not equal to target class %s,it will be deleted automatically",
		           *loaded_game->GetClass()->GetName(), *target_cls->GetName());
		return;
	}

	IWashSaveInterface* wash_save = Cast<IWashSaveInterface>(loaded_game);
	if (wash_save)
	{
		_AddOrRefreshStorage(wash_save);
	}
}

void UWashSaveSystem::_FinishTask(ESaveRequestPhase TargetPhase, TDeque<FSaveGameRequest>& InRequests)
{
	if (TargetPhase == ESaveRequestPhase::Delete)
	{
		while (InRequests.Num() > 0)
		{
			auto&& first_task = InRequests.First();
			if (first_task.IsFinished)
			{
				_RemoveStorage(first_task.SaveSlot);

				LOG_SAVE("delete %s finished %s", *first_task.SaveSlot,
				         first_task.IsSuccess ? TEXT("success") : TEXT("failed"));

				first_task.OnCompleteDelegate(first_task);
				InRequests.PopFirst();
			}
			else
			{
				break;
			}
		}
	}
	else if (TargetPhase == ESaveRequestPhase::Save)
	{
		while (InRequests.Num() > 0)
		{
			auto&& first_task = InRequests.First();
			if (first_task.IsFinished)
			{
				_PostProcessSaveTask(first_task);

				LOG_SAVE("save %s finished %s", *first_task.SaveSlot,
				         first_task.IsSuccess ? TEXT("success") : TEXT("failed"));

				first_task.OnCompleteDelegate(first_task);
				InRequests.PopFirst();
			}
			else
			{
				break;
			}
		}
	}
	else if (TargetPhase == ESaveRequestPhase::Load)
	{
		while (InRequests.Num() > 0)
		{
			auto&& first_task = InRequests.First();
			if (first_task.IsFinished)
			{
				//加载成功，需要把数据写入到列表中
				auto&& save_data = _FindSave(first_task.SaveSlot);
				if (save_data == nullptr)
				{
					USaveGame* loaded_game = UGameplayStatics::LoadGameFromMemory(first_task.Bytes);
					if (UWashSaveSystem::ProcessFunction != nullptr)
					{
						loaded_game = UWashSaveSystem::ProcessFunction(loaded_game, first_task.SaveSlot);
					}

					auto loaded_interface = Cast<IWashSaveInterface>(loaded_game);
					if (loaded_interface != save_data)
					{
						save_data->CopyFrom(loaded_game);
					}
				}

				first_task.OnCompleteDelegate(first_task);
				InRequests.PopFirst();
			}
			else
			{
				break;
			}
		}
	}
	else
	{
		WARN_SAVE("unknow phase %s", *BattleLibNativeUtil::EnumToFName(TargetPhase).ToString());
	}
}

void UWashSaveSystem::_RemoveTask(const FString& SlotName, ESaveRequestPhase TargetPhase)
{
	auto& target_tasks = _FindTaskGroup(TargetPhase);
	auto task_queue = target_tasks.Find(SlotName);
	if (task_queue == nullptr)
	{
		return;
	}

	if (task_queue->Num() == 0)
	{
		target_tasks.Remove(SlotName);
		return;
	}

	auto&& first_task = task_queue->First();
	first_task.OnCompleteDelegate(first_task);
	task_queue->PopFirst();
}

void UWashSaveSystem::_ClearTask(const FString& SlotName, ESaveRequestPhase TargetPhase)
{
	auto& target_tasks = _FindTaskGroup(TargetPhase);
	auto task_queue = target_tasks.Find(SlotName);
	if (task_queue == nullptr)
	{
		return;
	}

	if (task_queue->Num() == 0)
	{
		target_tasks.Remove(SlotName);
		return;
	}

	//clear 不回调
	// for (auto& task : task_queue)
	// {
	// 	task.OnCompleteDelegate.Broadcast( task);
	// }
	//
	// task_queue->Reset();
	//清空任务
	target_tasks.Remove(SlotName);
}

bool UWashSaveSystem::_FindRequest(const FString& SlotName, ESaveRequestPhase TargetPhase, FSaveGameRequest& OutRequest)
{
	auto&& target_tasks = _FindTaskGroup(TargetPhase);
	for (auto&& [task_path,task_queue] : target_tasks)
	{
		if (task_queue.Num() == 0)
		{
			continue;
		}
		auto&& first_task = task_queue.First();
		if (first_task.SaveSlot == SlotName && _HasPhase(first_task, TargetPhase))
		{
			OutRequest = first_task;
			return true;
		}
	}
	return false;
}

bool UWashSaveSystem::_UpdateRequest(const FString& SlotName, ESaveRequestPhase TargetPhase,
                                     const TFunctionRef<void(FSaveGameRequest&)> InFunc)
{
	auto&& target_tasks = _FindTaskGroup(TargetPhase);
	for (auto&& [task_path,task_queue] : target_tasks)
	{
		if (task_queue.Num() == 0)
		{
			continue;
		}
		auto&& first_task = task_queue.First();
		if (first_task.SaveSlot == SlotName && _HasPhase(first_task, TargetPhase))
		{
			InFunc(first_task);
			return true;
		}
	}
	return false;
}

void UWashSaveSystem::_AddOrRefreshStorage(IWashSaveInterface* in_save_interface)
{
	//检查数据有效性
	auto&& save_game = Cast<USaveGame>(in_save_interface);
	if (save_game == nullptr)
	{
		ERROR_SAVE("AddSave Failed  SaveGame is nullptr");
		return;
	}

	//检查索引表中是否存在
	auto&& slot_name = in_save_interface->GetSlotName();
	auto&& index_ptr = Storage_SaveIndexes.Find(slot_name);

	//refresh
	in_save_interface->SetSlotName(slot_name);

	//检查是否是有效索引，不是的话，添加/更新索引
	if (index_ptr == nullptr)
	{
		//执行添加
		TWeakObjectPtr<UObject> save_weak_ptr = MakeWeakObjectPtr(in_save_interface->_getUObject());

		auto&& index = Storage_SaveGames.Emplace(save_weak_ptr);
		Storage_SaveIndexes.Emplace(slot_name, index);
		Storage_Persistence_SaveGames.Add(in_save_interface->_getUObject());

		LOG_SAVE("AddSave To Storge %s success it's a newer save", *slot_name);
		return;
	}

	auto mapping_index = *index_ptr;

	if (Storage_SaveGames.IsValidIndex(mapping_index))
	{
		// auto save_weak_ptr = Storage_SaveGames[mapping_index];
		// if (save_weak_ptr.IsValid() == false)
		// {
		// 	ERROR_SAVE("savegame stored in index %d is invalid", mapping_index);
		// 	Storage_SaveGames[mapping_index] = MakeWeakObjectPtr(InSaveGame);
		// }
		// else
		// {
		// 	
		// }
		TWeakObjectPtr<UObject> save_weak_ptr = Storage_SaveGames[mapping_index];
		auto save_game_interface = Cast<IWashSaveInterface>(save_weak_ptr.Get());
		//如果不是同类型的存档，直接覆盖
		if (save_weak_ptr.IsValid() && save_game_interface && save_weak_ptr.Get()->GetClass() == save_game->GetClass())
		{
			if (save_game_interface != in_save_interface)
			{
				save_game_interface->CopyFrom(save_game);
				LOG_SAVE("refresh savegame stored in index %d savegame:%s", mapping_index, *slot_name);
			}
		}
		else
		{
			Storage_SaveGames[mapping_index] = MakeWeakObjectPtr(in_save_interface->_getUObject());
			Storage_Persistence_SaveGames.Add(Storage_SaveGames[mapping_index].Get());
			LOG_SAVE("refresh savegame stored in index %d savegame:%s", mapping_index, *slot_name);
		}
	}
	else
	{
		TWeakObjectPtr<UObject> save_weak_ptr = MakeWeakObjectPtr(in_save_interface->_getUObject());

		auto&& index = Storage_SaveGames.Emplace(save_weak_ptr);
		Storage_SaveIndexes.Emplace(slot_name, index);

		Storage_Persistence_SaveGames.Add(in_save_interface->_getUObject());

		LOG_SAVE("AddSave To Storge %s success it's a newer save", *slot_name);
	}
}

void UWashSaveSystem::_RemoveStorage(const FString& SlotName)
{
	auto&& index_ptr = Storage_SaveIndexes.Find(SlotName);
	if (index_ptr == nullptr)
	{
		return;;
	}
	//索引表中删除
	Storage_SaveIndexes.Remove(SlotName);
	//删除实际save game
	auto index = *index_ptr;

	if (Storage_SaveGames.IsValidIndex(index) == false)
	{
		return;
	}
	auto&& save_game = Storage_SaveGames[index];
	if (save_game.IsValid())
	{
		Storage_Persistence_SaveGames.Remove(save_game.Get());
	}
	Storage_SaveGames.RemoveAt(index);
}

void UWashSaveSystem::_IteratorAllRequests(const TFunctionRef<void(TMap<FString, TDeque<FSaveGameRequest>>&)> InFunc)
{
	InFunc(SavingTasks);
	InFunc(DeletingTasks);
	InFunc(LoadingTasks);
}

TMap<FString, TDeque<FSaveGameRequest>>& UWashSaveSystem::_FindTaskGroup(ESaveRequestPhase TargetPhase)
{
	if (_HasPhase(TargetPhase, ESaveRequestPhase::Save))
	{
		return SavingTasks;
	}
	else if (_HasPhase(TargetPhase, ESaveRequestPhase::Delete))
	{
		return DeletingTasks;
	}
	else
	{
		return LoadingTasks;
	}
}


void UWashSaveSystem::OnAsyncComplete(TFunction<void()> Callback)
{
	// NB. Using Ticker because AsyncTask may run during async package loading which may not be suitable for save data
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
		[Callback = MoveTemp(Callback)](float) -> bool
		{
			Callback();
			return false;
		}
	));
}

void UWashSaveSystem::_SaveGameAsync(bool bAttemptToUseUI, const TCHAR* Name, int32 PlatformUserId,
                                     FSaveGameRequest& InRequest,
                                     ISaveGameSystem::FSaveGameAsyncOpCompleteCallback Callback)
{
	FString SlotName(Name);
	if (InRequest.Bytes.Num() > 0)
	{
		TSharedRef<TArray<uint8>> save_bytes(new TArray<uint8>());
		// start the save operation on a background thread.
		AsyncTaskPipe.Launch(UE_SOURCE_LOCATION,
		                     [this, bAttemptToUseUI, SlotName, PlatformUserId, save_bytes, Callback]()
		                     {
			                     // save the savegame
			                     const bool bResult = GetPlatformSaveSystem()->SaveGame(
				                     bAttemptToUseUI, *SlotName, PlatformUserId, *save_bytes);

			                     // trigger the callback on the game thread
			                     if (Callback)
			                     {
				                     OnAsyncComplete(
					                     [SlotName, PlatformUserId, bResult, Callback]()
					                     {
						                     FPlatformUserId plat_user_id = FPlatformMisc::GetPlatformUserForUserIndex(
							                     0);
						                     Callback(SlotName, plat_user_id, bResult);
					                     }
				                     );
			                     }
		                     }
		);
	}
	else if (IsValid(InRequest.SaveGame))
	{
		IWashSaveInterface* save_game_interface = Cast<IWashSaveInterface>(InRequest.SaveGame);
		if (save_game_interface)
		{
			auto&& cloned_save_game = NewObject<USaveGame>(GetTransientPackage(), InRequest.SaveGame->GetClass());
			auto&& cloned_save_interface = Cast<IWashSaveInterface>(cloned_save_game);
			cloned_save_interface->CopyFrom(InRequest.SaveGame);
			cloned_save_game->AddToRoot();

			// start the save operation on a background thread.
			AsyncTaskPipe.Launch(UE_SOURCE_LOCATION,
			                     [this, bAttemptToUseUI, SlotName, PlatformUserId, cloned_save_game, Callback]()
			                     {
				                     TSharedRef<TArray<uint8>> save_bytes(new TArray<uint8>());

				                     TArray<uint8> mem_bytes;
				                     UGameplayStatics::SaveGameToMemory(cloned_save_game, mem_bytes);


				                     save_bytes->Append(mem_bytes);
				                     // save the savegame
				                     const bool bResult = GetPlatformSaveSystem()->SaveGame(
					                     bAttemptToUseUI, *SlotName, PlatformUserId, *save_bytes);

				                     // trigger the callback on the game thread
				                     if (Callback)
				                     {
					                     OnAsyncComplete(
						                     [SlotName, bResult,cloned_save_game, Callback]()
						                     {
							                     FPlatformUserId plat_user_id =
								                     FPlatformMisc::GetPlatformUserForUserIndex(0);
							                     cloned_save_game->RemoveFromRoot();
							                     Callback(SlotName, plat_user_id, bResult);
						                     }
					                     );
				                     }
			                     }
			);
		}
		else
		{
			TSharedRef<TArray<uint8>> save_bytes(new TArray<uint8>());

			TArray<uint8> mem_bytes;
			UGameplayStatics::SaveGameToMemory(InRequest.SaveGame, mem_bytes);
			// start the save operation on a background thread.
			AsyncTaskPipe.Launch(UE_SOURCE_LOCATION,
			                     [this, bAttemptToUseUI, SlotName, PlatformUserId, save_bytes, Callback]()
			                     {
				                     // save the savegame
				                     const bool bResult = GetPlatformSaveSystem()->SaveGame(
					                     bAttemptToUseUI, *SlotName, PlatformUserId, *save_bytes);

				                     // trigger the callback on the game thread
				                     if (Callback)
				                     {
					                     OnAsyncComplete(
						                     [SlotName, bResult, Callback]()
						                     {
							                     FPlatformUserId plat_user_id =
								                     FPlatformMisc::GetPlatformUserForUserIndex(0);
							                     Callback(SlotName, plat_user_id, bResult);
						                     }
					                     );
				                     }
			                     }
			);
		}
	}
}

void UWashSaveSystem::_LoadGameAsync(bool bAttemptToUseUI, const TCHAR* Name, FPlatformUserId PlatformUserId,
                                     ISaveGameSystem::FSaveGameAsyncLoadCompleteCallback Callback)
{
	FString SlotName(Name);

	// start the load operation on a background thread.
	AsyncTaskPipe.Launch(UE_SOURCE_LOCATION,
	                     [this, bAttemptToUseUI, SlotName, PlatformUserId, Callback]()
	                     {
		                     // load the savegame
		                     TSharedRef<TArray<uint8>> Data = MakeShared<TArray<uint8>>();
		                     int32 UserIndex = FPlatformMisc::GetUserIndexForPlatformUser(PlatformUserId);
		                     const bool bResult = GetPlatformSaveSystem()->LoadGame(
			                     bAttemptToUseUI, *SlotName, UserIndex, Data.Get());

		                     // trigger the callback on the game thread
		                     if (Callback)
		                     {
			                     // Callback(SlotName, PlatformUserId, bResult, Data.Get());
			                     OnAsyncComplete(
				                     [SlotName, PlatformUserId, bResult, Callback, Data]()
				                     {
					                     Callback(SlotName, PlatformUserId, bResult, Data.Get());
				                     }
			                     );
		                     }
	                     }
	);
}


void UWashSaveSystem::_DeleteGameAsync(bool bAttemptToUseUI, const TCHAR* Name, FPlatformUserId PlatformUserId,
                                       ISaveGameSystem::FSaveGameAsyncOpCompleteCallback Callback)
{
	FString SlotName(Name);

	// start the delete operation on a background thread.
	AsyncTaskPipe.Launch(UE_SOURCE_LOCATION,
	                     [this, bAttemptToUseUI, SlotName, PlatformUserId, Callback]()
	                     {
		                     int32 UserIndex = FPlatformMisc::GetUserIndexForPlatformUser(PlatformUserId);
		                     const bool bResult = GetPlatformSaveSystem()->DeleteGame(
			                     bAttemptToUseUI, *SlotName, UserIndex);

		                     // trigger the callback on the game thread
		                     if (Callback)
		                     {
			                     Callback(SlotName, PlatformUserId, bResult);
			                     // _OnAsyncComplete(
			                     //  [SlotName, PlatformUserId, bResult, Callback]()
			                     //  {
			                     //   Callback(SlotName, PlatformUserId, bResult);
			                     //  }
			                     // );
		                     }
	                     }
	);
}

// void UWashSaveSystem::_OnAsyncComplete(TFunction<void()> Callback)
// {
// 	// NB. Using Ticker because AsyncTask may run during async package loading which may not be suitable for save data
// 	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
// 		[Callback = MoveTemp(Callback)](float) -> bool
// 		{
// 			Callback();
// 			return false;
// 		}
// 	));
// }

bool UWashSaveSystem::_HasPhase(const FString& SlotName, ESaveRequestPhase TargetPhase)
{
	FSaveGameRequest request;
	if (_FindRequest(SlotName, TargetPhase, request))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool UWashSaveSystem::_HasPhase(FSaveGameRequest& Request, ESaveRequestPhase TargetPhase)
{
	return _HasPhase(Request.Phase, TargetPhase);
}

bool UWashSaveSystem::_HasPhase(ESaveRequestPhase Phase, ESaveRequestPhase TargetPhase)
{
	return (Phase & TargetPhase) == TargetPhase;
}
