
#pragma once
#include "Core/BattleLogPredefine.h"
BATTLEGAMEPLAYLIBRARY_API DECLARE_LOG_CATEGORY_EXTERN(BattleCONFIG, Log, All);
#define LOG_CONFIG(format, ...) BATTLE_BASELOG(BattleCONFIG, Log, TEXT(format), ##__VA_ARGS__)
#define WARN_CONFIG(format, ...) BATTLE_BASELOG(BattleCONFIG, Warning, TEXT(format), ##__VA_ARGS__)
#define ERROR_CONFIG(format, ...) BATTLE_BASELOG(BattleCONFIG, Error, TEXT(format), ##__VA_ARGS__)
#define ASSERT_CONFIG(condition, format, ...) ensureMsgf(condition, TEXT(format), ##__VA_ARGS__)


