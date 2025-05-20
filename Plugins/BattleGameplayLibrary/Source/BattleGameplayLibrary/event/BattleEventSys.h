// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "EventContext.h"
#include "EventData.h"
#include "BattleGamePlayLibrary/Template/BattleTemplate.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BattleEventSys.generated.h"

class EventContextCache
{
public:
	int32 EventId = 0;
	Next::EventContext Context;

	EventContextCache() = default;
	EventContextCache(int32 ID, Next::EventContext&& Data);
};

// 事件子系统, 生命周期全局在,由于Subsystem不支持Update, 我们需要在使用Timer创建定时器更新, 
// 因此继承UTickableWorldSubsystem, 在Initialize中初始化相应的缓存(暂时不考虑)
UCLASS()
class BATTLEGAMEPLAYLIBRARY_API UBattleEventSys : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// 事件id -> 事件列表字典
	TMap<int32, Next::FEventData> EventMap;
	// 尝试接触绑定字典
	TMap<int32, TArray<int32>> TryUnbindMap;
	//事件参数队列
	TQueue<EventContextCache, EQueueMode::Mpsc> EventQueue;
	//绑定等待队列
	TArray<Next::FEventBindData> BindEventQueue;

	//目前不是多线程暂时用不到这样 TLockFreePointerListLIFO<Next::FEventOwnerInfo>
	TArray<Next::FEventOwnerInfo> OwnerStack;

	// 最大事件发出次数
	int MaxEmitCount = 500;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Clear();
	virtual void Deinitialize() override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	void Tick(UWorld* World, ELevelTick TickType, float DeltaTime);


	void _DirectBindEvent(Next::FEventBindData& BindData);
	void BindEvent(int32 EventId, Next::EventFunc Function, const Next::FEventListenerData::FunctionInfo& FuncInfo);

	void UnbindEvent(int32 EventId, int32 HandleId);

	template <typename T>
	void UnbindAllEvent(T* Owner)
	{
		if (!Owner)
		{
			return;
		}
		uint64 Address = reinterpret_cast<uint64>(Owner);
		if constexpr (TIsUClassType<T>::Value)
		{
			for (auto&& [EventId,Data] : EventMap)
			{
				for (auto&& Listener : Data.Listeners)
				{
					//非UObject跳过
					if (!Listener.FunctionData.bUObject)
						continue;
					//同一个对象或者invalid
					if (Listener.FunctionData.FunctionOwnerAddress == Address || !Listener.FunctionData.WeakFunctionOwner.IsValid())
					{
						UnbindEvent(EventId, Listener.HandleID);
					}
				}
			}
		}
		else
		{
			for (auto&& [EventId,Data] : EventMap)
			{
				for (auto&& Listener : Data.Listeners)
				{
					//UObject跳过
					if (Listener.FunctionData.bUObject)
						continue;
					//同一个对象或者invalid
					if (Listener.FunctionData.FunctionOwnerAddress == Address || !Listener.FunctionData.WeakFunctionOwner.IsValid())
					{
						UnbindEvent(EventId, Listener.HandleID);
					}
				}
			}
		}
	}

	void Emit(int32 EventId, Next::EventContext&& Handle);

	//给个拷贝
	Next::FEventOwnerInfo GetPeek();
	void PushStack(Next::FEventOwnerInfo* Info);
	void PopStack();
};
