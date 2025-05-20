// Fill out your copyright notice in the Description page of Project Settings.


#include "rg_iteminst_object.h"

#include "Iris/ReplicationSystem/ReplicationFragmentUtil.h"
#include "RandomGod/Inventory/rg_itemdef_object.h"


#if UE_WITH_IRIS
#include "Iris/ReplicationSystem/ReplicationFragmentUtil.h"
#endif // UE_WITH_IRIS

#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(rg_iteminst_object)

URGInventoryItemInstance::URGInventoryItemInstance(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	
}
void URGInventoryItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, StatTags);
	DOREPLIFETIME(ThisClass, ItemDef);
}

#if UE_WITH_IRIS
void URGInventoryItemInstance::RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags)
{
	using namespace UE::Net;

	// Build descriptors and allocate PropertyReplicationFragments for this object
	FReplicationFragmentUtil::CreateAndRegisterFragmentsForObject(this, Context, RegistrationFlags);
}
#endif // UE_WITH_IRIS


void URGInventoryItemInstance::AddStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.AddStack(Tag, StackCount);
}

void URGInventoryItemInstance::RemoveStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	StatTags.RemoveStack(Tag, StackCount);

}

int32 URGInventoryItemInstance::GetStatTagStackCount(FGameplayTag Tag) const
{
	return  StatTags.GetStackCount(Tag);
}

bool URGInventoryItemInstance::HasStatTag(FGameplayTag Tag) const
{
	return StatTags.ContainsTag(Tag);
}

const URGInventoryItemFragment* URGInventoryItemInstance::FindFragmentByClass(
	TSubclassOf<URGInventoryItemFragment> FragmentClass) const
{
	if ((ItemDef != nullptr) && (FragmentClass != nullptr))
	{
		return GetDefault<URGInventoryItemDefinition>(ItemDef)->FindFragmentByClass(FragmentClass);
	}

	return nullptr;
}


void URGInventoryItemInstance::SetItemDef(TSubclassOf<URGInventoryItemDefinition> InDef)
{
	ItemDef = InDef;
}

