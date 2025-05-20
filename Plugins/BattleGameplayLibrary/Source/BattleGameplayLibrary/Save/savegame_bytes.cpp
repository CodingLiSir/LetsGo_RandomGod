// Fill out your copyright notice in the Description page of Project Settings.


#include "savegame_bytes.h"


void USaveGameBytes::CopyFrom(const USaveGame* InSaveGame)
{
	const USaveGameBytes* save_game_bytes = Cast<USaveGameBytes>(InSaveGame);
	if (save_game_bytes)
	{
		Bytes = save_game_bytes->Bytes;
	}
}

void USaveGameBytes::SetSlotName(const FString& InSlotName)
{
	SlotName = InSlotName;
}

FString USaveGameBytes::GetSlotName()
{
	return SlotName;
}


