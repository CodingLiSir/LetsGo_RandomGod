// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "save_worldsys.generated.h"

/**
 * 
 */
UCLASS()
class BATTLEGAMEPLAYLIBRARY_API USaveWorldSystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual TStatId GetStatId() const override;
	virtual  void Tick(float DeltaTime) override;
};
