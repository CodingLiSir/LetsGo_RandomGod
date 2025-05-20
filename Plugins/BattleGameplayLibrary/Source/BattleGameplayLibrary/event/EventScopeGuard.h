#pragma once
#include "BattleEventSys.h"
#include "BattleGamePlayLibrary/BattleLog/BattleLogEvent.h"
#include "BattleGamePlayLibrary/Template/BattleTemplate.h"

namespace Next
{
template <typename T, typename =typename TEnableIf<TIsUClassType<T>::Value>::type>
class EventScopeGuard final
{
private:
	TWeakObjectPtr<UBattleEventSys> Sys;
public:
	EventScopeGuard() = default;

	EventScopeGuard(UWorld* World, T* Target)
	{
		if (!IsValid(World))
		{
			ERROR_EVENT("EventScopeGuard Miss World")
			return;
		}

		if (!Target)
		{
			ERROR_EVENT("EventScopeGuard Miss TargetOwner")
			return;
		}
		Sys = World->GetSubsystem<UBattleEventSys>();
		if (Sys.IsValid())
		{
			FEventOwnerInfo Info;
			Info.Object = Target;
			Info.Type = stripped_type_name<T>();
			Sys->PushStack(&Info);
		}
	}

	~EventScopeGuard()
	{
		if (Sys.IsValid())
		{
			Sys->PopStack();
			Sys = nullptr;
		}
	}
};
}