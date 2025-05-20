// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BattleEventSys.h"
#include "EventContext.h"
#include "EventData.h"
#include "BattleGamePlayLibrary/BattleLog/BattleLogEvent.h"
PRAGMA_DISABLE_OPTIMIZATION
namespace Next::Event
{
	template <typename UserClass, typename TEnum, typename ParamType, typename ParamId, class = typename TEnableIf<
		          TIsEnum<TEnum>::Value>::Type>
	void BindEvent(UWorld* InWorld, TEnum EventId, void (*InFunc)(const ParamType*, ParamId))
	{
		if (!IsValid(InWorld))
		{
			ERROR_EVENT("Missing World")
			return;
		}
		UBattleEventSys* PEventSys = InWorld->GetSubsystem<UBattleEventSys>();
		if (!IsValid(PEventSys))
		{
			ERROR_EVENT("Missing UBattleEventSys")
			return;
		}
		using FuncType = decltype(InFunc);

		TWeakObjectPtr<UBattleEventSys> WeakSys(PEventSys);
		auto ThunkCallback = [WeakSys,InFunc](int32 EventId, int32 HandleId, EventContext& SenderPayload)
		{
			if (WeakSys.IsValid())
			{
				auto RawPtr = std::any_cast<ParamType>(&SenderPayload);
				check(RawPtr);
				if constexpr (TIsEnum<ParamId>::Value)
				{
					(*InFunc)(RawPtr, static_cast<TEnum>(EventId));
				}
				else
				{
					(*InFunc)(RawPtr, EventId);
				}
			}
		};
		Next::FEventListenerData::FunctionInfo FuncInfo;
		FuncInfo.bUObject = false;
		FuncInfo.FunctionAddress = reinterpret_cast<uint64>((void*&)InFunc);
#if WITH_EDITOR
		FuncInfo.FunctionName = stripped_type_name<FuncType>();
#endif
		//stripped_type_name<void (UserClass::*)(ContextHandler<ParamType>&, TEnum)>()
		return PEventSys->BindEvent(static_cast<int32>(EventId), ThunkCallback, FuncInfo);
	}

	template <typename UserClass, typename TEnum, typename ParamType, typename ParamId, class = typename TEnableIf<
		          TIsEnum<TEnum>::Value>::Type>
	void BindEvent(UWorld* InWorld, TEnum EventId, UserClass* InUserObject,
	               void (UserClass::*InFunc)(const ParamType*, ParamId))
	{
		if (!IsValid(InWorld))
		{
			ERROR_EVENT("Missing World")
			return;
		}
		UBattleEventSys* PEventSys = InWorld->GetSubsystem<UBattleEventSys>();
		if (!IsValid(PEventSys))
		{
			ERROR_EVENT("Missing UBattleEventSys")
			return;
		}
		using FuncType = decltype(InFunc);

		if constexpr (TIsUClassType<UserClass>::Value)
		{
			TWeakObjectPtr<UserClass> WeakObject(InUserObject);
			TWeakObjectPtr<UBattleEventSys> WeakSys(PEventSys);
			auto ThunkCallback = [WeakSys,WeakObject,InFunc](int32 EventId, int32 HandleId, EventContext& SenderPayload)
			{
				if (UserClass* StrongObject = WeakObject.Get())
				{
					auto RawPtr = std::any_cast<ParamType>(&SenderPayload);
					//EventContext 还有ContextHandler 可以做池，等pool完成
					if constexpr (TIsEnum<ParamId>::Value)
					{
						(StrongObject->*InFunc)(RawPtr, static_cast<TEnum>(EventId));
					}
					else
					{
						(StrongObject->*InFunc)(RawPtr, EventId);
					}
				}
				else if (UBattleEventSys* Sys = WeakSys.Get())
				{
					Sys->UnbindEvent(EventId, HandleId);
				}
			};

			Next::FEventListenerData::FunctionInfo FuncInfo;
			FuncInfo.bUObject = true;
			FuncInfo.FunctionAddress = reinterpret_cast<uint64>((void*&)InFunc);
#if WITH_EDITOR
			FuncInfo.FunctionName = stripped_type_name<FuncType>();
#endif
			FuncInfo.FunctionOwnerAddress = reinterpret_cast<uint64>(InUserObject);
			FuncInfo.WeakFunctionOwner = WeakObject;

			return PEventSys->BindEvent(static_cast<int32>(EventId), ThunkCallback, FuncInfo);
		}
		else
		{
			TWeakObjectPtr<UBattleEventSys> WeakSys(PEventSys);
			auto ThunkCallback = [WeakSys,InUserObject,InFunc](int32 EventId, int32 HandleId,
			                                                   EventContext& SenderPayload)
			{
				if (InUserObject)
				{
					auto RawPtr = std::any_cast<ParamType>(&SenderPayload);
					check(RawPtr);
					if constexpr (TIsEnum<ParamId>::Value)
					{
						(InUserObject->*InFunc)(RawPtr, static_cast<TEnum>(EventId));
					}
					else
					{
						(InUserObject->*InFunc)(RawPtr, EventId);
					}
				}
				else if (UBattleEventSys* Sys = WeakSys.Get())
				{
					Sys->UnbindEvent(EventId, HandleId);
				}
			};

			Next::FEventListenerData::FunctionInfo FuncInfo;
			FuncInfo.bUObject = false;
			FuncInfo.FunctionAddress = reinterpret_cast<uint64>((void*&)InFunc);
#if WITH_EDITOR
			FuncInfo.FunctionName = stripped_type_name<FuncType>();
#endif
			FuncInfo.FunctionOwnerAddress = reinterpret_cast<uint64>(InUserObject);

			return PEventSys->BindEvent(static_cast<int32>(EventId), ThunkCallback, FuncInfo);
		}
	}

	template <typename UserClass, typename TEnum, typename ParamType, class = typename TEnableIf<TIsEnum<
		          TEnum>::Value>::Type>
	void BindEvent(UWorld* InWorld, TEnum EventId, UserClass* InUserObject, void (UserClass::*InFunc)(const ParamType*))
	{
		if (!IsValid(InWorld))
		{
			ERROR_EVENT("Missing World")
			return;
		}
		UBattleEventSys* PEventSys = InWorld->GetSubsystem<UBattleEventSys>();
		if (!IsValid(PEventSys))
		{
			ERROR_EVENT("Missing UBattleEventSys")
			return;
		}
		using FuncType = decltype(InFunc);

		if constexpr (TIsUClassType<UserClass>::Value)
		{
			TWeakObjectPtr<UserClass> WeakObject(InUserObject);
			TWeakObjectPtr<UBattleEventSys> WeakSys(PEventSys);
			auto ThunkCallback = [WeakSys,WeakObject,InFunc](int32 EventId, int32 HandleId, EventContext& SenderPayload)
			{
				if (UserClass* StrongObject = WeakObject.Get())
				{
					if constexpr (TIsUClassType<ParamType>::Value)
					{
						UObject* RawPtr = std::any_cast<UObject>(&SenderPayload);
						auto CastedPtr = Cast<ParamType>(RawPtr);
						(StrongObject->*InFunc)(CastedPtr);
					}
					else
					{
						auto RawPtr = std::any_cast<ParamType>(&SenderPayload);
						auto str =stripped_type_unrealname<ParamType>();
						(StrongObject->*InFunc)(RawPtr);
					}
				}
				else if (UBattleEventSys* Sys = WeakSys.Get())
				{
					Sys->UnbindEvent(EventId, HandleId);
				}
			};

			Next::FEventListenerData::FunctionInfo FuncInfo;
			FuncInfo.bUObject = true;
			FuncInfo.FunctionAddress = reinterpret_cast<uint64>((void*&)InFunc);
#if WITH_EDITOR
			FuncInfo.FunctionName = stripped_type_name<FuncType>();
#endif
			FuncInfo.FunctionOwnerAddress = reinterpret_cast<uint64>(InUserObject);
			FuncInfo.WeakFunctionOwner = WeakObject;

			return PEventSys->BindEvent(static_cast<int32>(EventId), ThunkCallback, FuncInfo);
		}
		else
		{
			TWeakObjectPtr<UBattleEventSys> WeakSys(PEventSys);
			auto ThunkCallback = [WeakSys,InUserObject,InFunc](int32 EventId, int32 HandleId,
			                                                   EventContext& SenderPayload)
			{
				if (InUserObject)
				{
					auto RawPtr = std::any_cast<ParamType>(&SenderPayload);
					(InUserObject->*InFunc)(RawPtr);
				}
				else if (UBattleEventSys* Sys = WeakSys.Get())
				{
					Sys->UnbindEvent(EventId, HandleId);
				}
			};

			Next::FEventListenerData::FunctionInfo FuncInfo;
			FuncInfo.bUObject = false;
			FuncInfo.FunctionAddress = reinterpret_cast<uint64>((void*&)InFunc);
#if WITH_EDITOR
			FuncInfo.FunctionName = stripped_type_name<FuncType>();
#endif
			FuncInfo.FunctionOwnerAddress = reinterpret_cast<uint64>(InUserObject);

			return PEventSys->BindEvent(static_cast<int32>(EventId), ThunkCallback, FuncInfo);
		}
	}

	template <typename UserClass, typename TEnum, typename ParamType, class = typename TEnableIf<TIsEnum<
		          TEnum>::Value>::Type>
	void BindEvent(UWorld* InWorld, TEnum EventId, UserClass* InUserObject,
	               void (UserClass::*InFunc)(const ParamType**))
	{
		if (!IsValid(InWorld))
		{
			ERROR_EVENT("Missing World")
			return;
		}
		UBattleEventSys* PEventSys = InWorld->GetSubsystem<UBattleEventSys>();
		if (!IsValid(PEventSys))
		{
			ERROR_EVENT("Missing UBattleEventSys")
			return;
		}
		using FuncType = decltype(InFunc);

		if constexpr (TIsUClassType<UserClass>::Value)
		{
			TWeakObjectPtr<UserClass> WeakObject(InUserObject);
			TWeakObjectPtr<UBattleEventSys> WeakSys(PEventSys);
			auto ThunkCallback = [WeakSys,WeakObject,InFunc](int32 EventId, int32 HandleId, EventContext& SenderPayload)
			{
				if (UserClass* StrongObject = WeakObject.Get())
				{
					const ParamType** RawPtr = std::any_cast<const ParamType*>(&SenderPayload);
					(StrongObject->*InFunc)(RawPtr);
				}
				else if (UBattleEventSys* Sys = WeakSys.Get())
				{
					Sys->UnbindEvent(EventId, HandleId);
				}
			};

			Next::FEventListenerData::FunctionInfo FuncInfo;
			FuncInfo.bUObject = true;
			FuncInfo.FunctionAddress = reinterpret_cast<uint64>((void*&)InFunc);
#if WITH_EDITOR
			FuncInfo.FunctionName = stripped_type_name<FuncType>();
#endif
			FuncInfo.FunctionOwnerAddress = reinterpret_cast<uint64>(InUserObject);
			FuncInfo.WeakFunctionOwner = WeakObject;

			return PEventSys->BindEvent(static_cast<int32>(EventId), ThunkCallback, FuncInfo);
		}
		else
		{
			TWeakObjectPtr<UBattleEventSys> WeakSys(PEventSys);
			auto ThunkCallback = [WeakSys,InUserObject,InFunc](int32 EventId, int32 HandleId,
			                                                   EventContext& SenderPayload)
			{
				if (InUserObject)
				{
					const ParamType** RawPtr = std::any_cast<const ParamType*>(&SenderPayload);
					(InUserObject->*InFunc)(RawPtr);
				}
				else if (UBattleEventSys* Sys = WeakSys.Get())
				{
					Sys->UnbindEvent(EventId, HandleId);
				}
			};

			Next::FEventListenerData::FunctionInfo FuncInfo;
			FuncInfo.bUObject = false;
			FuncInfo.FunctionAddress = reinterpret_cast<uint64>((void*&)InFunc);
#if WITH_EDITOR
			FuncInfo.FunctionName = stripped_type_name<FuncType>();
#endif
			FuncInfo.FunctionOwnerAddress = reinterpret_cast<uint64>(InUserObject);

			return PEventSys->BindEvent(static_cast<int32>(EventId), ThunkCallback, FuncInfo);
		}
	}

	template <typename UserClass, typename TEnum, class = typename TEnableIf<TIsEnum<TEnum>::Value>::Type>
	void BindEvent(UWorld* InWorld, TEnum EventId, UserClass* InUserObject, void (UserClass::*InFunc)())
	{
		if (!IsValid(InWorld))
		{
			ERROR_EVENT("Missing World")
			return;
		}
		UBattleEventSys* PEventSys = InWorld->GetSubsystem<UBattleEventSys>();
		if (!IsValid(PEventSys))
		{
			ERROR_EVENT("Missing UBattleEventSys")
			return;
		}
		using FuncType = decltype(InFunc);

		if constexpr (TIsUClassType<UserClass>::Value)
		{
			TWeakObjectPtr<UserClass> WeakObject(InUserObject);
			TWeakObjectPtr<UBattleEventSys> WeakSys(PEventSys);
			auto ThunkCallback = [WeakSys,WeakObject,InFunc](int32 EventId, int32 HandleId, EventContext& SenderPayload)
			{
				if (UserClass* StrongObject = WeakObject.Get())
				{
					(StrongObject->*InFunc)();
				}
				else if (UBattleEventSys* Sys = WeakSys.Get())
				{
					Sys->UnbindEvent(EventId, HandleId);
				}
			};

			Next::FEventListenerData::FunctionInfo FuncInfo;
			FuncInfo.bUObject = true;
			FuncInfo.FunctionAddress = reinterpret_cast<uint64>((void*&)InFunc);
#if WITH_EDITOR
			FuncInfo.FunctionName = stripped_type_name<FuncType>();
#endif
			FuncInfo.FunctionOwnerAddress = reinterpret_cast<uint64>(InUserObject);
			FuncInfo.WeakFunctionOwner = WeakObject;

			return PEventSys->BindEvent(static_cast<int32>(EventId), ThunkCallback, FuncInfo);
		}
		else
		{
			TWeakObjectPtr<UBattleEventSys> WeakSys(PEventSys);
			auto ThunkCallback = [WeakSys,InUserObject,InFunc](int32 EventId, int32 HandleId,
			                                                   EventContext& SenderPayload)
			{
				if (InUserObject)
				{
					(InUserObject->*InFunc)(EventId);
				}
				else if (UBattleEventSys* Sys = WeakSys.Get())
				{
					Sys->UnbindEvent(EventId, HandleId);
				}
			};

			Next::FEventListenerData::FunctionInfo FuncInfo;
			FuncInfo.bUObject = false;
			FuncInfo.FunctionAddress = reinterpret_cast<uint64>((void*&)InFunc);
#if WITH_EDITOR
			FuncInfo.FunctionName = stripped_type_name<FuncType>();
#endif
			FuncInfo.FunctionOwnerAddress = reinterpret_cast<uint64>(InUserObject);

			return PEventSys->BindEvent(static_cast<int32>(EventId), ThunkCallback, FuncInfo);
		}
	}

	template <class T>
	void UnbindAllEvent(UWorld* InWorld, T* Owner)
	{
		if (!IsValid(InWorld))
		{
			return;
		}
		UBattleEventSys* PEventSys = InWorld->GetSubsystem<UBattleEventSys>();
		if (!IsValid(PEventSys))
		{
			return;
		}
		PEventSys->UnbindAllEvent(Owner);
	}

	template <class T>
	void UnbindAllEvent(T* Owner)
	{
		check(Owner != nullptr && Owner->GetWorld() != nullptr);
		UnbindAllEvent(Owner->GetWorld(), Owner);
	}

	template <class TParams, typename TEnum, class = typename TEnableIf<TIsEnum<TEnum>::Value>::Type>
	void EmitEvent(UWorld* InWorld, TEnum EventId, TParams&& Params)
	{
		if (!IsValid(InWorld))
		{
			WARN_EVENT("Missing World")
			return;
		}
		UBattleEventSys* PEventSys = InWorld->GetSubsystem<UBattleEventSys>();
		if (!IsValid(PEventSys))
		{
			WARN_EVENT("Missing UBattleEventSys")
			return;
		}

		Next::EventContext Context = Forward<TParams>(Params);

		LOG_EVENT("emit eventid:%d  params:%s", static_cast<int32>(EventId), *stripped_type_unrealname<TParams>())

		PEventSys->Emit(static_cast<int32>(EventId), MoveTemp(Context));
	}

	template <typename TEnum, class = typename TEnableIf<TIsEnum<TEnum>::Value>::Type>
	void EmitEvent(UWorld* InWorld, TEnum EventId)
	{
		if (!IsValid(InWorld))
		{
			WARN_EVENT("Missing World")
			return;
		}
		UBattleEventSys* PEventSys = InWorld->GetSubsystem<UBattleEventSys>();
		if (!IsValid(PEventSys))
		{
			WARN_EVENT("Missing UBattleEventSys")
			return;
		}
		LOG_EVENT("emit eventid:%d ", static_cast<int32>(EventId))
		PEventSys->Emit(static_cast<int32>(EventId), {});
	}
}

PRAGMA_ENABLE_OPTIMIZATION
