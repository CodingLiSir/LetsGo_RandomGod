// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RandomGod/System/GameplayTagStack.h"
#include "UObject/Object.h"
#include "Templates/SubclassOf.h"

#include "rg_iteminst_object.generated.h"

class URGInventoryItemFragment;
class URGInventoryItemDefinition;
struct FFrame;
struct FGameplayTag;

UCLASS(BlueprintType)
class RANDOMGOD_API URGInventoryItemInstance : public UObject
{
	GENERATED_BODY()

public:
	URGInventoryItemInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	//~UObject interface
	virtual bool IsSupportedForNetworking() const override { return true; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~End of UObject interface

	// 为StatTags中的目标Tag增加数量(小于1则不处理)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void AddStatTagStack(FGameplayTag Tag, int32 StackCount);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void RemoveStatTagStack(FGameplayTag Tag, int32 StackCount);

	// 查询标签数量(这里加const是表明不会对任何成员变量做修改，也避免编译器多余的检查)
	UFUNCTION(BlueprintCallable, Category=Inventory)
	int32 GetStatTagStackCount(FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, Category=Inventory)
	bool HasStatTag(FGameplayTag Tag) const;

	TSubclassOf<URGInventoryItemDefinition> GetItemDef() const
	{
		return ItemDef;
	}

	//重要：TSubclassOf：类型安全，可以确保你只能将一个指向基类或其派生类的指针分配给 TSubclassOf；
	UFUNCTION(BlueprintCallable, BlueprintPure=false, meta=(DeterminesOutputType=FragmentClass))
	const URGInventoryItemFragment* FindFragmentByClass(TSubclassOf<URGInventoryItemFragment> FragmentClass) const;

	//TODO 搞清楚这个模板函数存在的意义
	//模板函数：避免显式类型转换，
	//(ResultClass*)：强制类型转换，保证返回的是模版推导的类型
	template <typename ResultClass>
	const ResultClass* FindFragmentByClass() const
	{
		return (ResultClass*)FindFragmentByClass(ResultClass::StaticClass());
	}

private:
#if UE_WITH_IRIS //---用于控制 Unreal Engine 中的 Iris 网络系统的可用性， TODO 学习 Iris 网络系统：更高效的数据传输和网络状态同步
	/** Register all replication fragments */
	virtual void RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags) override;
#endif // UE_WITH_IRIS

	void SetItemDef(TSubclassOf<URGInventoryItemDefinition> InDef);

	friend struct FRGInventoryList;
	
private:
	//TODO:用途是什么？好像是用于存储每个物品的状态及其数量？
	UPROPERTY(Replicated)
	FGameplayTagStackContainer StatTags;

	// 定义只有一个，定义中包含的碎片可以有根据需求配置很多个
	UPROPERTY(Replicated)
	TSubclassOf<URGInventoryItemDefinition> ItemDef;
};
