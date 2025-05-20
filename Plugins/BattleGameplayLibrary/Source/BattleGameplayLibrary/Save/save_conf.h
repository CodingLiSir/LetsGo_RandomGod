// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "UObject/Object.h"
#include "save_conf.generated.h"

USTRUCT(BlueprintType)
struct FWashSaveConf
{
	GENERATED_BODY()
	
	/**
	 * @brief Save的槽位名
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Save)
	FString SaveSlotName = "";

	/**
	 * @brief Save的用户索引
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Save)
	int32 SaveUserIndex = 0;

	/**
	 * @brief 保存的目标类型
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Save)
	UClass* SaveUserCls = nullptr;
};

USTRUCT(BlueprintType)
struct FWashSaveConfInSettings
{
	GENERATED_BODY()

	/**
	 * @brief Save的槽位名
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Save)
	FString SaveSlotName = "";

	/**
	 * @brief 保存的目标类型
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Save)
	TSubclassOf<USaveGame> SaveUserCls ;
};