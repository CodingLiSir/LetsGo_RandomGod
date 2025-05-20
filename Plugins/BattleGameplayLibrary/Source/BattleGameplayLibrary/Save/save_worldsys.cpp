// Fill out your copyright notice in the Description page of Project Settings.


#include "save_worldsys.h"
#include "sys_save.h"

void USaveWorldSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void USaveWorldSystem::Deinitialize()
{
	Super::Deinitialize();
}

TStatId USaveWorldSystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USaveWorldSystem, STATGROUP_Tickables);
}

void USaveWorldSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	auto&& save_inst = UWashSaveSystem::Get();
	save_inst->_RunTasks();
}
