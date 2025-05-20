// Fill out your copyright notice in the Description page of Project Settings.


#include "rg_inventorycomp_actorcomp.h"

#include "rg_itemdef_object.h"
#include "rg_iteminst_object.h"
#include "Engine/World.h"
#include "NativeGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h" 




FString FRGInventoryEntry::GetDebugString() const
{
	TSubclassOf<URGInventoryItemFragment> ItemDef;
	if (Instance != nullptr)
	{
		ItemDef = Instance->GetItemDef();
	}

	return FString::Printf(TEXT("%s (%d x %s)"), *GetNameSafe(Instance), StackCount, *GetNameSafe(ItemDef));
}

TArray<URGInventoryItemInstance*> FRGInventoryList::GetAllItems() const
{
	TArray<URGInventoryItemInstance*> Results;
	Results.Reserve(Entries.Num());//一次开辟足够内存，避免每次Add都动态扩容
	for (const FRGInventoryEntry& Entry : Entries)
	{
		if (Entry.Instance != nullptr) 
		{
			Results.Add(Entry.Instance);
		}
	}
	return Results;
}

void FRGInventoryList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		FRGInventoryEntry& Stack = Entries[Index];
		BroadcastChangeMessage(Stack, /*OldCount=*/ Stack.StackCount, /*NewCount=*/ 0);
		Stack.LastObservedCount = 0;
	}
}

void FRGInventoryList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		FRGInventoryEntry& Stack = Entries[Index];
		BroadcastChangeMessage(Stack, /*OldCount=*/ 0, /*NewCount=*/ Stack.StackCount);
		Stack.LastObservedCount = Stack.StackCount;
	}
}

void FRGInventoryList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		FRGInventoryEntry& Stack = Entries[Index];
		check(Stack.LastObservedCount != INDEX_NONE);
		BroadcastChangeMessage(Stack, /*OldCount=*/ Stack.LastObservedCount, /*NewCount=*/ Stack.StackCount);
		Stack.LastObservedCount = Stack.StackCount;
	}
}

URGInventoryItemInstance* FRGInventoryList::AddEntry(TSubclassOf<URGInventoryItemDefinition> ItemDef,
	int32 StackCount)
{
	URGInventoryItemInstance* Result = nullptr;

	check(ItemDef != nullptr);
	check(OwnerComponent);

	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());

	
	FRGInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();//AddDefaulted_GetRef：返回新添加元素的引用，方便---添加后立即对这个元素进行修改或操作，而无需额外的索引查找步骤
	NewEntry.Instance = NewObject<URGInventoryItemInstance>(OwnerComponent->GetOwner()); 
	NewEntry.Instance->SetItemDef(ItemDef);
	for (URGInventoryItemFragment* Fragment : GetDefault<URGInventoryItemDefinition>(ItemDef)->Fragments)
	{
		if (Fragment != nullptr)
		{
			//这里多态实现
			Fragment->OnInstanceCreated(NewEntry.Instance);
		}
	}
	NewEntry.StackCount = StackCount;
	Result = NewEntry.Instance;

	MarkItemDirty(NewEntry);//FFastArraySerializer规则：添加或者改变，必须调用

	return Result;
}

void FRGInventoryList::AddEntry(URGInventoryItemInstance* Instance)
{
	unimplemented();
}

void FRGInventoryList::RemoveEntry(URGInventoryItemInstance* Instance)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt) //你可以直接操作迭代器，包括修改它的递增方式、使用不同的遍历策略（反向遍历）
	{
		FRGInventoryEntry& Entry = *EntryIt;
		if (Entry.Instance == Instance)
		{
			EntryIt.RemoveCurrent();
			MarkArrayDirty();//FFastArraySerializer规则：数组删除，必须调用
		}
	}
}

void FRGInventoryList::BroadcastChangeMessage(FRGInventoryEntry& Entry, int32 OldCount, int32 NewCount)
{

	
	FRGInventoryChangeMessage Message;
	Message.InventoryOwner = OwnerComponent;
	Message.Instance = Entry.Instance;
	Message.NewCount = NewCount;
	Message.Delta = NewCount - OldCount;

	//TODO 这里是利用Tag事件系统进行广播（背包stack数量改变）
	// UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(OwnerComponent->GetWorld());
	// MessageSystem.BroadcastMessage(TAG_Lyra_Inventory_Message_StackChanged, Message);
}

URGInventoryComponent::URGInventoryComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, InventoryList(this)
{
	SetIsReplicatedByDefault(true);
}

void URGInventoryComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, InventoryList);
}


bool URGInventoryComponent::CanAddItemDefinition(TSubclassOf<URGInventoryItemDefinition> ItemDef, int32 StackCount)
{
	//@TODO: Add support for stack limit / uniqueness checks / etc...
	return true;
}

URGInventoryItemInstance* URGInventoryComponent::AddItemDefinition(TSubclassOf<URGInventoryItemDefinition> ItemDef,
                                                                   int32 StackCount)
{
	URGInventoryItemInstance* Result = nullptr;
	if (ItemDef != nullptr)
	{
		Result = InventoryList.AddEntry(ItemDef, StackCount);
		
		if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && Result)
		{
			AddReplicatedSubObject(Result);
		}
	}
	return Result;
}

void URGInventoryComponent::AddItemInstance(URGInventoryItemInstance* ItemInstance)
{
	InventoryList.AddEntry(ItemInstance);//这个没实现
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && ItemInstance)
	{
		AddReplicatedSubObject(ItemInstance);
	}
}

void URGInventoryComponent::RemoveItemInstance(URGInventoryItemInstance* ItemInstance)
{
	InventoryList.RemoveEntry(ItemInstance);

	if (ItemInstance && IsUsingRegisteredSubObjectList())//IsUsingRegisteredSubObjectList默认是true
	{
		//见280行：因为URGInventoryComponent→InventoryList.contain（ItemInstance）所以ItemInstance属于URGInventoryComponent的一个SubObject
		RemoveReplicatedSubObject(ItemInstance); 
	}
}

TArray<URGInventoryItemInstance*> URGInventoryComponent::GetAllItems() const
{
	return InventoryList.GetAllItems();
}

URGInventoryItemInstance* URGInventoryComponent::FindFirstItemStackByDefinition(
	TSubclassOf<URGInventoryItemDefinition> ItemDef) const
{
	for (const FRGInventoryEntry& Entry : InventoryList.Entries)
	{
		URGInventoryItemInstance* Instance = Entry.Instance;

		if (IsValid(Instance))
		{
			if (Instance->GetItemDef() == ItemDef)
			{
				return Instance;
			}
		}
	}

	return nullptr;
}

int32 URGInventoryComponent::GetTotalItemCountByDefinition(TSubclassOf<URGInventoryItemDefinition> ItemDef) const
{
	int32 TotalCount = 0;
	for (const FRGInventoryEntry& Entry : InventoryList.Entries)
	{
		URGInventoryItemInstance* Instance = Entry.Instance;

		if (IsValid(Instance))
		{
			if (Instance->GetItemDef() == ItemDef)
			{
				++TotalCount;
			}
		}
	}

	return TotalCount;
}

bool URGInventoryComponent::ConsumeItemsByDefinition(TSubclassOf<URGInventoryItemDefinition> ItemDef,
	int32 NumToConsume)
{
	AActor* OwningActor = GetOwner();
	if (!OwningActor || !OwningActor->HasAuthority())
	{
		return false;
	}

	//@TODO: N squared right now as there's no acceleration structure
	int32 TotalConsumed = 0;
	while (TotalConsumed < NumToConsume)
	{
		if (URGInventoryItemInstance* Instance = URGInventoryComponent::FindFirstItemStackByDefinition(ItemDef))
		{
			InventoryList.RemoveEntry(Instance);
			++TotalConsumed;
		}
		else
		{
			return false;
		}
	}

	return TotalConsumed == NumToConsume;
}

bool URGInventoryComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (FRGInventoryEntry& Entry : InventoryList.Entries)
	{
		URGInventoryItemInstance* Instance = Entry.Instance;

		if (Instance && IsValid(Instance))
		{
			//这个与或的想表示的是：是否表示至少有一个对象成功复制了。
			WroteSomething |= Channel->ReplicateSubobject(Instance, *Bunch, *RepFlags);
		}
	}

	return WroteSomething;
}

void URGInventoryComponent::ReadyForReplication()
{
	Super::ReadyForReplication();

	// Register existing ULyraInventoryItemInstance
	if (IsUsingRegisteredSubObjectList())
	{
		for (const FRGInventoryEntry& Entry : InventoryList.Entries)
		{
			URGInventoryItemInstance* Instance = Entry.Instance;

			if (IsValid(Instance))
			{
				AddReplicatedSubObject(Instance);
			}
		}
	}
}
