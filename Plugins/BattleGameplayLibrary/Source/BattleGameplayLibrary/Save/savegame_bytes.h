// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BattleGamePlayLibrary/Save/save_interface.h"
#include "GameFramework/SaveGame.h"
#include "UObject/Object.h"
#include "savegame_bytes.generated.h"

UCLASS(Blueprintable, BlueprintType)
class BATTLEGAMEPLAYLIBRARY_API USaveGameBytes : public USaveGame, public IWashSaveInterface
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<uint8> Bytes;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString SlotName;

	virtual void CopyFrom(const USaveGame* InSaveGame) override;
	virtual void SetSlotName(const FString& InSlotName) override;
	virtual FString GetSlotName() override;
};
