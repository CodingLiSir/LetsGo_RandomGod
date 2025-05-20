// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Deque.h"
#include "SaveGameSystem.h"
#include "save_conf.h"
#include "save_request.h"
#include "sys_save.generated.h"

enum class ESaveRequestPhase:uint8;


UENUM()
enum class ESaveSystemPhase :uint8
{
	Invalid,
	//完全初始化完成
	Initilized,
	//初始化过程中并且等待刷新完成
	InitilizedButNotRefresh,
	// //正在刷新
	// Refreshing,
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRefreshComplete);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInitomplete);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWashOpDelegate, const FString&, slotname, bool, isSuccess);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FWashLoadDelegate, const FString&, slotname,
                                               TScriptInterface<IWashSaveInterface>, savedata, bool, isSuccess);

class IWashSaveInterface;

UCLASS()
class BATTLEGAMEPLAYLIBRARY_API UWashSaveSystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	static UE::Tasks::FPipe AsyncTaskPipe;
	TFunction<USaveGame*(USaveGame* InSaveGame, const FString& SlotName)> ProcessFunction;
	ISaveGameSystem* PlatFormSaveSystem;

	static UWashSaveSystem* Get();

	ESaveSystemPhase SystemPhase;

	//存档请求列表
	TMap<FString, TDeque<FSaveGameRequest>> SavingTasks;

	//正在删除的存档
	TMap<FString, TDeque<FSaveGameRequest>> DeletingTasks;

	//正在加载的存档
	TMap<FString, TDeque<FSaveGameRequest>> LoadingTasks;

	UPROPERTY()
	TSet<UClass*> AvailableClasses;

	//存档配置列表
	UPROPERTY()
	TArray<FWashSaveConf> SaveConfList;

	//所有的存档数据,key为文件名
	UPROPERTY()
	TMap<FString, int32> Storage_SaveIndexes;

	//为了让weakptr那边能够持久化存储
	UPROPERTY()
	TSet<UObject*> Storage_Persistence_SaveGames;

	//真实的存档数据
	TSparseArray<TWeakObjectPtr<UObject>> Storage_SaveGames;

	FOnInitomplete OnLoadComplete;
	FOnRefreshComplete OnRefreshComplete;
	//委托
	ISaveGameSystem::FSaveGameAsyncOpCompleteCallback OnSaveGameDelegate;
	ISaveGameSystem::FSaveGameAsyncOpCompleteCallback OnDeleteGameDelegate;
	ISaveGameSystem::FSaveGameAsyncLoadCompleteCallback OnLoadGameDelegate;


	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	void Init(const	TFunction<USaveGame*(USaveGame* InSaveGame, const FString& SlotName)>& InFunc,ISaveGameSystem* InSys);

	ISaveGameSystem* GetPlatformSaveSystem();

	void _InitSystem();
	void ScanSaveGames();
	bool IsValidSave(UClass* InSaveGame);

	ESaveRequestPhase GetPhase(const FString& SlotName);

	bool Exist(const FString& SlotName);
	void Filter(const TFunctionRef<bool(const FString& SlotName, IWashSaveInterface* InSaveGame)> InFilterFunc,
	            TArray<IWashSaveInterface*>& OutSlotNames);

	bool ValidateSaveData(IWashSaveInterface* InSaveGame);
	bool ValidateSaveData(const FString& SlotName, TArray<uint8>& InBytes, UClass* InClass);

	int32 SaveSync(IWashSaveInterface* InSaveGame);
	int32 SaveSync(const FString& SlotName, TArray<uint8>& InBytes, UClass* InClass);
	int32 SaveAsync(const FString& SlotName, IWashSaveInterface* InSaveGame,
	                FWashOpDelegate OnComplete = FWashOpDelegate());
	int32 SaveAsync(const FString& SlotName, TArray<uint8>& InBytes, UClass* InClass,
	                FWashOpDelegate OnComplete = FWashOpDelegate());

	int32 LoadSync(const FString& SlotName,OUT IWashSaveInterface*& OutSaveGame);
	int32 LoadAsync(const FString& SlotName, FWashLoadDelegate OnComplete = FWashLoadDelegate());

	int32 DeleteSync(IWashSaveInterface* InSaveGame);
	int32 DeleteSync(const FString& SlotName);
	int32 DeleteAsync(IWashSaveInterface* InSaveGame, FWashOpDelegate OnComplete = FWashOpDelegate());
	int32 DeleteAsync(const FString& SlotName, FWashOpDelegate OnComplete = FWashOpDelegate());

	FString _GetSaveGamePathConf(const UClass* InCls);
	UClass* _GetSaveGameClass(const FString& SlotName);
	IWashSaveInterface* _FindSave(int32 Index);
	IWashSaveInterface* _FindSave(const FString& SlotName);

	void _RunTasks();
	void _RunTask(FSaveGameRequest& InRequest);
	void _PostProcessSaveTask(FSaveGameRequest& first_task);

	void _FinishTask(ESaveRequestPhase TargetPhase, TDeque<FSaveGameRequest>& InRequests);
	bool _FindRequest(const FString& SlotName, ESaveRequestPhase TargetPhase,OUT FSaveGameRequest& OutRequest);
	bool _UpdateRequest(const FString& SlotName, ESaveRequestPhase TargetPhase,
	                    const TFunctionRef<void(FSaveGameRequest&)> InFunc);
	void _RemoveTask(const FString& SlotName, ESaveRequestPhase TargetPhase);
	void _ClearTask(const FString& SlotName, ESaveRequestPhase TargetPhase);
	void _AddOrRefreshStorage(IWashSaveInterface* in_save_interface);
	void _RemoveStorage(const FString& SlotName);
	void _IteratorAllRequests(const TFunctionRef<void(TMap<FString, TDeque<FSaveGameRequest>>&)> InFunc);

	TMap<FString, TDeque<FSaveGameRequest>>& _FindTaskGroup(ESaveRequestPhase TargetPhase);
	void OnAsyncComplete(TFunction<void()> Callback);

	void _CleanGameData();

	void _SaveGameAsync(bool bAttemptToUseUI, const TCHAR* Name, int32 PlatformUserId,
	                    FSaveGameRequest& Data,
	                    ISaveGameSystem::FSaveGameAsyncOpCompleteCallback Callback);
	void _LoadGameAsync(bool bAttemptToUseUI, const TCHAR* Name, FPlatformUserId PlatformUserId,
	                    ISaveGameSystem::FSaveGameAsyncLoadCompleteCallback Callback);
	void _DeleteGameAsync(bool bAttemptToUseUI, const TCHAR* Name, FPlatformUserId PlatformUserId,
	                      ISaveGameSystem::FSaveGameAsyncOpCompleteCallback Callback);
	// void _OnAsyncComplete(TFunction<void()> Callback);

	bool _HasPhase(const FString& SlotName, ESaveRequestPhase TargetPhase);
	bool _HasPhase(FSaveGameRequest& Request, ESaveRequestPhase TargetPhase);
	bool _HasPhase(ESaveRequestPhase Phase, ESaveRequestPhase TargetPhase);
};
