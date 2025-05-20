// Fill out your copyright notice in the Description page of Project Settings.


#include "BattleEventSys.h"
#include "BattleGamePlayLibrary/BattleLog/BattleLogEvent.h"
#include "BattleGamePlayLibrary/Command/GamePlayCmd.h"
#include "Kismet/KismetSystemLibrary.h"

CSV_DECLARE_CATEGORY_MODULE_EXTERN(BATTLEGAMEPLAYLIBRARY_API, NextEvent);

EventContextCache::EventContextCache(int32 ID, Next::EventContext&& Data)
{
	EventId = ID;
	Context = MoveTemp(Data);
}

void UBattleEventSys::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	//注册事件
	FWorldDelegates::OnWorldPostActorTick.AddUObject(this, &UBattleEventSys::Tick);

	LOG_EVENT("Battle Event Sys Init...");
}

void UBattleEventSys::Deinitialize()
{
	FWorldDelegates::OnWorldPostActorTick.RemoveAll(this);
	//这里要补泄漏检查
	Clear();

	Super::Deinitialize();

	LOG_EVENT("Battle Event Sys Deinitialize...");
}

bool UBattleEventSys::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::Editor;
}


void UBattleEventSys::Tick(UWorld* World, ELevelTick TickType, float DeltaTime)
{
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(NextEvent);


	// 0. 遍历绑定事件队列
	for (int32 i = 0; i < BindEventQueue.Num(); ++i)
	{
		auto&& bind_data = BindEventQueue[i];
		_DirectBindEvent(bind_data);
	}
	
	BindEventQueue.Reset();

	
	// 1.遍历移除事件字典
	for (auto&& [UnbindEventId,UnbindHandleIds] : TryUnbindMap)
	{
		auto&& EventPtr = EventMap.Find(UnbindEventId);
		if (!EventPtr)
		{
			continue;
		}
		//遍历找到匹配的进行删除
		for (auto&& HandleId : UnbindHandleIds)
		{
#if WITH_EDITOR
			bool bRemoved = false;
#endif
			for (int32 i = 0; i < EventPtr->Listeners.Num(); ++i)
			{
				auto&& Listener = EventPtr->Listeners[i];
				// 1.1  移除事件监听
				if (Listener.HandleID == HandleId)
				{
#if WITH_EDITOR
					bRemoved = true;
#if EVENT_SIG_OPTION
					if (Next::CVarShowEventLog.GetValueOnAnyThread() == 1)
						LOG_EVENT("WorldId :%d UnBind EventId = %d HandleId:%d Success", GetWorld()->GetUniqueID(),
					          UnbindEventId, HandleId);
#endif
#endif
					//listener的触发时序可能并不重要
					EventPtr->Listeners.RemoveAtSwap(i);
					// EventPtr->Listeners.RemoveAt(i);

					break;
				}
			}
#if WITH_EDITOR
			if (!bRemoved)
			{
				if (Next::CVarShowEventLog.GetValueOnAnyThread() == 1)
					WARN_EVENT("WorldId :%d UnBind EventId = %d HandleId:%d Failed  Num:%d", GetWorld()->GetUniqueID(),
				           UnbindEventId, HandleId, EventPtr->Listeners.Num());
			}
#endif
		}
	}

	// 1.2 清空移除字典
	TryUnbindMap.Reset();
	// 2. 遍历事件队列
	EventContextCache EventCache;
	int32 idx = 0;
	for (; idx < MaxEmitCount && !EventQueue.IsEmpty(); idx++)
	{
		if (!EventQueue.Dequeue(EventCache))
			break;
		// 3.1 检测事件是否存在 EventMap中
		auto&& PEvent = EventMap.Find(EventCache.EventId);
		if (PEvent)
		{
			// 3.2 存在则发送事件
			for (auto&& Listener : PEvent->Listeners)
			{
				Listener.ReceivedCallback(EventCache.EventId, Listener.HandleID, EventCache.Context);
			}
		}
	}
}

void UBattleEventSys::_DirectBindEvent(Next::FEventBindData& BindData)
{

	// 1. 检测是否在事件列表中
	auto&& EventDataPtr = EventMap.FindOrAdd(BindData.EventId);

	//判断重复注册
	for (auto&& Listener : EventDataPtr.Listeners)
	{
		if (Listener.FunctionData.FunctionAddress == BindData.FuncInfo.FunctionAddress &&
			Listener.FunctionData.FunctionOwnerAddress == BindData.FuncInfo.FunctionOwnerAddress)
		{
			if (Next::CVarShowEventLog.GetValueOnAnyThread() == 1)
				WARN_EVENT("BindEvent Has Same Delegate For Event:%d",BindData. EventId)
			return ;
		}
	}

	auto&& Entry = EventDataPtr.Listeners.AddDefaulted_GetRef();
	Entry.ReceivedCallback = MoveTemp(BindData.Function);
	Entry.FunctionData =BindData. FuncInfo;
	Entry.HandleID = ++EventDataPtr.UniqueID;
	Entry.EventOwnerInfo = GetPeek();


#if WITH_EDITOR && EVENT_SIG_OPTION
	if (Next::CVarShowEventLog.GetValueOnAnyThread() == 1)
		LOG_EVENT("WorldId :%d BindEvent EventId = %d HandleId:%d", GetWorld()->GetUniqueID(), BindData.EventId,
			  Entry.HandleID);
#endif
}


void UBattleEventSys::BindEvent(int32 EventId, Next::EventFunc Function,
													  const Next::FEventListenerData::FunctionInfo& FuncInfo)
{
	Next::FEventBindData bind_data;
	bind_data.EventId = EventId;
	bind_data.Function = MoveTemp(Function);
	bind_data.FuncInfo = FuncInfo;
	
	BindEventQueue.Emplace(bind_data);
}


void UBattleEventSys::UnbindEvent(int32 EventId, int32 HandleId)
{
	// 检测是否在事件字典中
	auto&& ResultArray = TryUnbindMap.FindOrAdd(EventId);
	ResultArray.Emplace(HandleId);
	
#if WITH_EDITOR
	if (Next::CVarShowEventLog.GetValueOnAnyThread() == 1)
		LOG_EVENT("WorldId :%d TryUnbindEvent EventId = %d HandleId:%d  ", GetWorld()->GetUniqueID(), EventId, HandleId);
#endif
}

void UBattleEventSys::Emit(int32 EventId, Next::EventContext&& Handle)
{
	// 1. 安全检查
	// 2. 将参数组装成结构体
	// 3. 将结构体放入队列
	EventQueue.Enqueue({EventId, MoveTemp(Handle)});
}

Next::FEventOwnerInfo UBattleEventSys::GetPeek()
{
	if (OwnerStack.Num() > 0)
	{
		return OwnerStack[OwnerStack.Num() - 1];
	}
	else
	{
		return {};
	}
}

void UBattleEventSys::PushStack(Next::FEventOwnerInfo* Info)
{
	if (Info)
		OwnerStack.Push(*Info);
}

void UBattleEventSys::PopStack()
{
	OwnerStack.Pop();
}

void UBattleEventSys::Clear()
{
	// 1. 尝试结束绑定字典
	TryUnbindMap.Reset();
	// 3. 清除事件发送队列
	EventQueue.Empty();

	OwnerStack.Empty();
}

