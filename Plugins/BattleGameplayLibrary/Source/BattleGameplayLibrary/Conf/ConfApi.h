#pragma once
#include "ConfSys.h"

namespace Next
{
	//输入唯一key, 获取相应DataTable中的行, 如果不存在则返回无效的TWeakPtr即可
	template <typename T>
	static T* GetConf(FName Key)
	{
		UConfSys* Inst = UConfSys::GetInst();
		if (Inst)
		{
			return Inst->GetConf<T>(Key);
		}
		WARN_CONFIG("GetConf Failed UConfSys没有初始化");
		return nullptr;
	}

	template <typename T>
	static bool ContainsConf(FName Key)
	{
		UConfSys* Inst = UConfSys::GetInst();
		if (Inst)
		{
			return Inst->ContainsConf<T>(Key);
		}
		return false;
	}

	template <typename T>
	static T* GetConfRaw(uint32 TypeHash, const FName& Key)
	{
		UConfSys* Inst = UConfSys::GetInst();
		if (Inst)
		{
			FTableRowBase* RowBasePtr = Inst->GetConfRaw(TypeHash, Key);
			if (RowBasePtr)
			{
				return static_cast<T*>(RowBasePtr);
			}
		}

		WARN_CONFIG("GetConf Failed UConfSys没有初始化");
		return nullptr;
	}

	// 获得一个配置表行的名字的列表
	template <typename T>
	static TArray<FName> GetRawNameArray()
	{
		UConfSys* Inst = UConfSys::GetInst();
		if (Inst)
		{
			return Inst->GetRawNameArray<T>();
		}
		WARN_CONFIG("GetConf Failed UConfSys没有初始化");
		return TArray<FName>();
	}

	static TArray<FName> GetRawNameArray(uint32 TypeHash)
	{
		UConfSys* Inst = UConfSys::GetInst();
		if (Inst)
		{
			return Inst->GetRawNameArray(TypeHash);
		}
		WARN_CONFIG("GetConf Failed UConfSys没有初始化");
		return TArray<FName>();
	}

	template <typename T>
	static void TryFindRows(TMap<FName, T*>& OutRowMap, TFunction<bool(T&)> FilterFunctor)
	{
		UConfSys* Inst = UConfSys::GetInst();
		if (Inst == nullptr)
		{
			WARN_CONFIG("GetConf Failed UConfSys没有初始化");
			return;
		}

		Inst->TryFindRows(OutRowMap, FilterFunctor);
	}

	template <class T>
	static void GetAllRows(TArray<T*>& OutRowArray)
	{
		UConfSys* Inst = UConfSys::GetInst();
		if (Inst == nullptr)
		{
			WARN_CONFIG("GetConf Failed UConfSys没有初始化");
			return;
		}

		Inst->GetAllRows(OutRowArray);
	}

	//TODO: 输入过滤器函数, 遍历所有相同类型的DataTable数据, 如果不存在则返回空TArray即可
	// template <typename T>
	// static TArray<T*> GetAllConfByFilter(TFunction<bool(T&)> FilterFunctor) {}
}
