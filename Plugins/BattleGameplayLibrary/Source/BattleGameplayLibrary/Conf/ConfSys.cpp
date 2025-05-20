#include "ConfSys.h"
#include "Engine/AssetManager.h"

UConfSys* UConfSys::Inst = nullptr;

UConfSys* UConfSys::GetInst()
{
	return Inst;
}


void UConfSys::Initialize(FSubsystemCollectionBase& Collection)
{
	Inst = this;
	Super::Initialize(Collection);
}

void UConfSys::Deinitialize()
{
	Super::Deinitialize();
	DataBase.Reset();
	Inst = nullptr;
}


TArray<FName> UConfSys::GetRawNameArray(uint32 TypeHash)
{
	//查表
	UDataTable** DataTablePtrIt = DataBase.Find(TypeHash);
	if (DataTablePtrIt == nullptr)
	{
		ERROR_CONFIG("<UConfSys> GetConf查表失败, 表格不存在: %d", TypeHash)
		return TArray<FName>();
	}
	//取数据
	const UDataTable* DataTablePtr = *DataTablePtrIt;
	if (DataTablePtr == nullptr)
	{
		ERROR_CONFIG("<UConfSys> GetConf查表失败, 表格资源为空: %d", TypeHash)
		return TArray<FName>();
	}


	return DataTablePtr->GetRowNames();
}

FTableRowBase* UConfSys::GetConfRaw(uint32 TypeHash, FName RowName)
{
	//查表
	UDataTable** DataTablePtrIt = DataBase.Find(TypeHash);
	if (DataTablePtrIt == nullptr)
	{
		ERROR_CONFIG("<UConfSys> GetConf查表失败, 表格不存在: %d", TypeHash)
		return nullptr;
	}
	//取数据
	const UDataTable* DataTablePtr = *DataTablePtrIt;
	if (DataTablePtr == nullptr)
	{
		ERROR_CONFIG("<UConfSys> GetConf查表失败, 表格资源为空: %d", TypeHash)
		return nullptr;
	}

	static FString ContextStr = TEXT("UConfSys"); // 给内部打印用的
	FTableRowBase* RowPtr = DataTablePtr->FindRow<FTableRowBase>(RowName, ContextStr);
	if (RowPtr == nullptr)
		ERROR_CONFIG("<UConfSys> GetConfRaw寻找Row失败, Hash = %d, RowName = %s", TypeHash, *RowName.ToString());
	return RowPtr;
}

void UConfSys::Add(uint32 TypeHash, UDataTable* Data)
{
	check(TypeHash != 0);
	check(Data != nullptr);

	DataBase.Emplace(TypeHash, Data);
	LOG_CONFIG("<UConfSys> Add TypeHash: %d, UDataTable: %s", TypeHash, *Data->GetName())
}

void UConfSys::Empty()
{
	DataBase.Empty();
}
