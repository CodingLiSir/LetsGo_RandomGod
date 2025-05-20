// Fill out your copyright notice in the Description page of Project Settings.


#include "RGGameData.h"

#include "RGAssetManager.h"

URGGameData::URGGameData()
{
}

const URGGameData& URGGameData::Get()
{
	return URGAssetManager::Get().GetGameData();
}
