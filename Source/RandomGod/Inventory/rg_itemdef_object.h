// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "rg_itemdef_object.generated.h"

class ULyraInventoryItemInstance;
class URGInventoryItemInstance;
struct FFrame;


//碎片：一个及以上碎片组成一个物品定义
UCLASS(DefaultToInstanced, EditInlineNew, Abstract)
class RANDOMGOD_API URGInventoryItemFragment : public UObject
{
	GENERATED_BODY()

public:
	virtual void OnInstanceCreated(URGInventoryItemInstance* Instance) const {}
};

//一个物品定义由很多碎片组成，这个类的实例是只读（因为Const宏）
UCLASS(Blueprintable, Const, Abstract)
class RANDOMGOD_API URGInventoryItemDefinition : public UObject
{
	GENERATED_BODY()

public:
	
	URGInventoryItemDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display)
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display, Instanced)
	TArray<TObjectPtr<URGInventoryItemFragment>> Fragments;

public:
	
	const URGInventoryItemFragment* FindFragmentByClass(TSubclassOf<URGInventoryItemFragment> FragmentClass) const;
};
