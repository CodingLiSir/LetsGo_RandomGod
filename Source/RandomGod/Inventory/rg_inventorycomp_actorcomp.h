// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "rg_inventorycomp_actorcomp.generated.h"


class URGInventoryComponent;
class URGInventoryItemInstance;
class URGInventoryItemDefinition;

struct FRGInventoryList;
struct FNetDeltaSerializeInfo;


USTRUCT(BlueprintType)
struct FRGInventoryChangeMessage
{
	GENERATED_BODY()
	
	//问：为什么要包含InventoryOwner？ 猜测：这个ChangeMessage由InventoryOwner提供
	UPROPERTY(BlueprintReadOnly,Category= Inventory)
	TObjectPtr<UActorComponent> InventoryOwner = nullptr;
	
	//问：为什么要包含Instance？猜测：通过这个实例与背包容器中进行比对
	UPROPERTY(BlueprintReadOnly,Category= Inventory)
	TObjectPtr<URGInventoryItemInstance> Instance = nullptr;
	
	UPROPERTY(BlueprintReadOnly,Category=Inventory)
	int32 NewCount = 0;

	//问：为什么要包含Delta？猜测：差值，用于记录数量变化量
	UPROPERTY(BlueprintReadOnly,Category=Inventory)
	int32 Delta = 0;
};

//单个Item信息，包含Item Inst，准备将其作为容器元素（容器继承于FFastArraySerializer）
USTRUCT(BlueprintType)
struct FRGInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FRGInventoryEntry()
	{}

	//主动调用输出Entry的相关信息，属于覆写FFastArraySerializerItem中GetDebugString方法
	FString GetDebugString() const;

private:
	// 这两个friend声明原因：持有关系高→低 URGInventoryComponent→FRGInventoryList→FRGInventoryEntry
	friend FRGInventoryList;
	friend URGInventoryComponent;

	UPROPERTY()
	TObjectPtr<URGInventoryItemInstance> Instance = nullptr;

	UPROPERTY()
	int32 StackCount = 0;

	//TODO 记录这个字段的用途？
	UPROPERTY(NotReplicated)
	int32 LastObservedCount = INDEX_NONE;
};

//用于装载Item的容器
USTRUCT(BlueprintType)
struct FRGInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()
	FRGInventoryList()
	: OwnerComponent(nullptr)
	{
	}
	FRGInventoryList(UActorComponent* InOwnerComponent)
		: OwnerComponent(nullptr)
	{
	}
	
	TArray<URGInventoryItemInstance*> GetAllItems() const;

public:
	//~FFastArraySerializer contract 使用合约，必须实现
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);//*在复制期间删除元素之前调用（FFastArraySerializer自动调用）
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);//*在复制期间添加元素之前调用（FFastArraySerializer自动调用）
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);//*在复制期间改变元素之前调用（FFastArraySerializer自动调用）
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FRGInventoryEntry, FRGInventoryList>(Entries, DeltaParms, *this);
	}

	//TODO 问：这里增删方法为什么以URGInventoryItemInstance作为参数？而不是FRGInventoryEntry？
	URGInventoryItemInstance* AddEntry(TSubclassOf<URGInventoryItemDefinition> ItemDef, int32 StackCount);
	void AddEntry(URGInventoryItemInstance* Instance);
	void RemoveEntry(URGInventoryItemInstance* Instance);

private:
    //TODO 搞清楚这个是做什么的，好像很重要
	void BroadcastChangeMessage(FRGInventoryEntry& Entry, int32 OldCount, int32 NewCount);
	
private:
	friend URGInventoryComponent;

private:
	// Replicated list of items
	UPROPERTY()
	TArray<FRGInventoryEntry> Entries;

	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;
};




//TStructOpsTypeTraitsBase2:反射系统的结构体模板,用于指定结构体的特定行为，例如是否支持序列化、复制构造等。
template<>
struct TStructOpsTypeTraits<FRGInventoryList> : public TStructOpsTypeTraitsBase2<FRGInventoryList>
{
	enum { WithNetDeltaSerializer = true };//WithNetDeltaSerializer:定义序列化方式，表示采用增量序列化
};



UCLASS(BlueprintType,Blueprintable,ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class RANDOMGOD_API URGInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URGInventoryComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	bool CanAddItemDefinition(TSubclassOf<URGInventoryItemDefinition> ItemDef, int32 StackCount = 1);
	
	//Authority调用，TODO 为什么传入URGInventoryItemDefinition，返回URGInventoryItemInstance？
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	URGInventoryItemInstance* AddItemDefinition(TSubclassOf<URGInventoryItemDefinition> ItemDef, int32 StackCount = 1);

	//这个方法是预留的，没实现别调
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void AddItemInstance(URGInventoryItemInstance* ItemInstance);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void RemoveItemInstance(URGInventoryItemInstance* ItemInstance);

	UFUNCTION(BlueprintCallable, Category=Inventory, BlueprintPure=false)
	TArray<URGInventoryItemInstance*> GetAllItems() const;

	UFUNCTION(BlueprintCallable, Category=Inventory, BlueprintPure)
	URGInventoryItemInstance* FindFirstItemStackByDefinition(TSubclassOf<URGInventoryItemDefinition> ItemDef) const;

	int32 GetTotalItemCountByDefinition(TSubclassOf<URGInventoryItemDefinition> ItemDef) const;
	//消耗：也就是Remove
	bool ConsumeItemsByDefinition(TSubclassOf<URGInventoryItemDefinition> ItemDef, int32 NumToConsume);

	//~UObject interface
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	virtual void ReadyForReplication() override;
	//~End of UObject interface
	
private:
	
	UPROPERTY(Replicated)
	FRGInventoryList InventoryList;
};
