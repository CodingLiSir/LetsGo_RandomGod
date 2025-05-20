// Fill out your copyright notice in the Description page of Project Settings.


#include "rg_itemdef_object.h"

#include "Templates/SubclassOf.h"
#include "UObject/ObjectPtr.h"

//是一种包含生成代码的优化方式。它会内联包含与指定类（或其他类型）相关的生成代码，而不是通过传统的生成 .gen.cpp 文件来单独编译。这种方法能显著减少引擎和项目的编译时间
#include UE_INLINE_GENERATED_CPP_BY_NAME(rg_itemdef_object)


URGInventoryItemDefinition::URGInventoryItemDefinition(const FObjectInitializer& ObjectInitializer)
:Super(ObjectInitializer)
{
	
}

const URGInventoryItemFragment* URGInventoryItemDefinition::FindFragmentByClass(
	TSubclassOf<URGInventoryItemFragment> FragmentClass) const
{
	if (FragmentClass == nullptr)
		return nullptr;
	
	for (auto Fragment : Fragments)
	{
		if (Fragment && Fragment->IsA(FragmentClass))
		{
			return Fragment;
		}
	}
	
	return nullptr;
}

