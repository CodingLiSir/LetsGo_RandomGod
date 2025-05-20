
#pragma once
#include "Core/BattleLogPredefine.h"
BATTLEGAMEPLAYLIBRARY_API DECLARE_LOG_CATEGORY_EXTERN(BattleRG, Log, All);
#define LOG_RG(format, ...) BATTLE_BASELOG(BattleRG, Log, TEXT(format), ##__VA_ARGS__)
#define WARN_RG(format, ...) BATTLE_BASELOG(BattleRG, Warning, TEXT(format), ##__VA_ARGS__)
#define ERROR_RG(format, ...) BATTLE_BASELOG(BattleRG, Error, TEXT(format), ##__VA_ARGS__)
#define ASSERT_RG(condition, format, ...) ensureMsgf(condition, TEXT(format), ##__VA_ARGS__)


