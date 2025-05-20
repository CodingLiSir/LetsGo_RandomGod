
#pragma once
#include "Core/BattleLogPredefine.h"
BATTLEGAMEPLAYLIBRARY_API DECLARE_LOG_CATEGORY_EXTERN(BattleSAVE, Log, All);
#define LOG_SAVE(format, ...) BATTLE_BASELOG(BattleSAVE, Log, TEXT(format), ##__VA_ARGS__)
#define WARN_SAVE(format, ...) BATTLE_BASELOG(BattleSAVE, Warning, TEXT(format), ##__VA_ARGS__)
#define ERROR_SAVE(format, ...) BATTLE_BASELOG(BattleSAVE, Error, TEXT(format), ##__VA_ARGS__)
#define ASSERT_SAVE(condition, format, ...) ensureMsgf(condition, TEXT(format), ##__VA_ARGS__)


