#pragma once
#include "EventContext.h"

namespace Next
{
	using EventFunc = TFunction<void(int32, int32, Next::EventContext&)>;

	//用来控制显示debug调试信息
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
#define EVENT_SIG_OPTION 1
#else
#define EVENT_SIG_OPTION 0
#endif

	struct FEventOwnerInfo
	{
		const ANSICHAR* Type;
		TWeakObjectPtr<UObject> Object;

		bool HasOwner()
		{
			return Object.IsValid();
		}
	};

	struct FEventListenerData
	{
		struct FunctionInfo
		{
			bool bUObject = false;

			TWeakObjectPtr<UObject> WeakFunctionOwner;

			/** 原始的对象地址**/
			uint64 FunctionOwnerAddress = 0;

			/** 原始的函数地址**/
			uint64 FunctionAddress = 0;
#if WITH_EDITOR
			FName FunctionName;
#endif
		};

		/** 消息事件回调 **/
		EventFunc ReceivedCallback = nullptr;

		/** 函数信息 **/
		FunctionInfo FunctionData;
		/** 消息handle的 ID **/
		int32 HandleID = 0;

		/** Owner消息 只是用于定向发消息 **/
		FEventOwnerInfo EventOwnerInfo;
	};

	struct FEventData
	{
		/** 注册的消息列表**/
		TArray<FEventListenerData> Listeners;
		//计数器，自增给FEventListenerData 创建所属的唯一Id
		int32 UniqueID = 0;
	};

	struct FEventBindData
	{
		int32 EventId;
		Next::EventFunc Function;
		Next::FEventListenerData::FunctionInfo FuncInfo;
	};
}
