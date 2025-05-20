// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "save_request.generated.h"

class USaveGame;

UENUM()
enum class ESaveRequestPhase :uint8
{
	Free = 0,
	Load = 1,
	Save = 1 << 1,
	Delete = 1 << 2,
	WriteBlock = 1 << 3,
	ReadBlock = 1 << 4,
};

USTRUCT()
struct FSaveGameRequest
{
	GENERATED_BODY()

public:
	bool IsFinished = false;

	bool IsSuccess = false;

	bool IsRunning = false;

	//创建时间
	double CreateTimeStamp = 0;

	ESaveRequestPhase Phase = ESaveRequestPhase::Free;

	FString SaveSlot;

	UPROPERTY()
	USaveGame* SaveGame = nullptr;

	TArray<uint8> Bytes;

	TFunction<void(FSaveGameRequest&)> OnCompleteDelegate;
};
