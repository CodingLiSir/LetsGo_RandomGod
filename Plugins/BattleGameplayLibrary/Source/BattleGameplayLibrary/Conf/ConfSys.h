#pragma once
#include "BattleGamePlayLibrary/BattleLog/BattleLogConfig.h"
#include "Engine/DataTable.h"
#include "ConfSys.generated.h"

//编辑器下都
UCLASS()
class BATTLEGAMEPLAYLIBRARY_API UConfSys : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	static UConfSys* Inst;
	static UConfSys* GetInst();

	UPROPERTY()
	TMap<uint32, UDataTable*> DataBase;

	//读取所有的DataTable数据, 暂时使用硬代码LoadObj
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 获得一个配置表行的名字的列表
	TArray<FName> GetRawNameArray(uint32 TypeHash);
	//不适用模板,直接给与Row的StructTypeHash, 以及RowName来获取数据
	FTableRowBase* GetConfRaw(uint32 TypeHash, FName RowName);

	void Add(uint32 TypeHash, UDataTable* Data);
	void Empty();

	//通过T::StaticStruct反射获得类型, 调用GetStructTypeHash来检查是合法, 获得相对应的DataTable, 根据RowName获得数据
	template <typename T>
	T* GetConf(const FName& RowName)
	{
		if (RowName.IsNone())
		{
			return nullptr;
		}

		const int32 TypeHash = GetTypeHash(T::StaticStruct());
		//查表
		UDataTable** DataTablePtrIt = DataBase.Find(TypeHash);
		if (DataTablePtrIt == nullptr)
		{
			WARN_CONFIG("<UConfSys> GetConf查表失败, 表格不存在: %s", *T::StaticStruct()->GetName())
			return nullptr;
		}
		//取数据
		const UDataTable* DataTablePtr = *DataTablePtrIt;
		if (DataTablePtr == nullptr)
		{
			WARN_CONFIG("<UConfSys> GetConf查表失败, 表格资源为空: %s", *T::StaticStruct()->GetName())
			return nullptr;
		}

		static FString ContextStr = TEXT("UConfSys"); // 给内部打印用的
		T* RowPtr = DataTablePtr->FindRow<T>(RowName, ContextStr);
		if (RowPtr == nullptr)
		{
			WARN_CONFIG("<UConfSys> GetConfigRaw寻找失败, Class = %s, RowName = %s"
			            , *T::StaticStruct()->GetName()
			            , *RowName.ToString());
		}
		return RowPtr;
	}

	template <typename T>
	bool ContainsConf(const FName& RowName)
	{
		if (RowName.IsNone())
		{
			return false;
		}

		const int32 TypeHash = GetTypeHash(T::StaticStruct());
		//查表
		UDataTable** DataTablePtrIt = DataBase.Find(TypeHash);
		if (DataTablePtrIt == nullptr)
		{
			return false;
		}
		//取数据
		const UDataTable* DataTablePtr = *DataTablePtrIt;
		if (DataTablePtr == nullptr)
		{
			return false;
		}

		static FString ContextStr = TEXT("UConfSys"); // 给内部打印用的
		T* RowPtr = DataTablePtr->FindRow<T>(RowName, ContextStr);
		if (RowPtr == nullptr)
		{
			return false;
		}
		return true;
	}


	// 获得一个配置表行的名字的列表
	template <typename T>
	TArray<FName> GetRawNameArray()
	{
		const uint32 TypeHash = GetTypeHash(T::StaticStruct());
		return GetRawNameArray(TypeHash);
	}


	template <typename T>
	UDataTable* GetLoadData()
	{
		const int32 TypeHash = GetTypeHash(T::StaticStruct());
		if (DataBase.Contains(TypeHash))
		{
			return DataBase[TypeHash];
		}
		return nullptr;
	}

	template <typename T>
	void TryFindRows(TMap<FName, T*>& OutRowMap, TFunction<bool (T&)> Filter)
	{
		const int32 TypeHash = GetTypeHash(T::StaticStruct());
		//查表
		UDataTable** DataTablePtrIt = DataBase.Find(TypeHash);
		if (DataTablePtrIt == nullptr)
		{
			ERROR_CONFIG("<UConfSys> GetConf查表失败, 表格不存在: %s", *T::StaticStruct()->GetName())
			return;
		}
		//取filter返回true的数据
		const UDataTable* DataTablePtr = *DataTablePtrIt;
		if (DataTablePtr == nullptr)
		{
			ERROR_CONFIG("<UConfSys> GetConf查表失败, 表格资源为空: %s", *T::StaticStruct()->GetName())
			return;
		}

		for (auto& pair : DataTablePtr->GetRowMap())
		{
			T* Value = reinterpret_cast<T*>(pair.Value);
			if (!Filter(*Value))
				continue;

			OutRowMap.Add(pair.Key, Value);
		}
	}

	template <class T>
	void GetAllRows(TArray<T*>& OutRowArray)
	{
		const int32 TypeHash = GetTypeHash(T::StaticStruct());
		//查表
		UDataTable** DataTablePtrIt = DataBase.Find(TypeHash);
		if (DataTablePtrIt == nullptr)
		{
			ERROR_CONFIG("<UConfSys> GetConf查表失败, 表格不存在: %s", *T::StaticStruct()->GetName())
			return;
		}

		//取数据
		const UDataTable* DataTablePtr = *DataTablePtrIt;
		if (DataTablePtr == nullptr)
		{
			ERROR_CONFIG("<UConfSys> GetConf查表失败, 表格资源为空: %s", *T::StaticStruct()->GetName())
			return;
		}

		DataTablePtr->GetAllRows(nullptr, OutRowArray);
	}
};
