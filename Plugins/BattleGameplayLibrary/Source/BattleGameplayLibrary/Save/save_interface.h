// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "UObject/Interface.h"
#include "save_interface.generated.h"


UINTERFACE()
class UWashSaveInterface : public UInterface
{
	GENERATED_BODY()
};


class BATTLEGAMEPLAYLIBRARY_API IWashSaveInterface
{
	GENERATED_BODY()

public:

	virtual void CopyFrom(const USaveGame* InSaveGame) =0;

	virtual FString GetSlotName() =0;

	virtual void SetSlotName(const FString& InSlotName) =0;
};
