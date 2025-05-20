// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Logging/LogMacros.h"

class UObject;

RANDOMGOD_API DECLARE_LOG_CATEGORY_EXTERN(LogRG, Log, All);
RANDOMGOD_API DECLARE_LOG_CATEGORY_EXTERN(LogRGExperience, Log, All);
RANDOMGOD_API DECLARE_LOG_CATEGORY_EXTERN(LogRGAbilitySystem, Log, All);
RANDOMGOD_API DECLARE_LOG_CATEGORY_EXTERN(LogRGTeams, Log, All);

RANDOMGOD_API FString GetClientServerContextString(UObject* ContextObject = nullptr);
