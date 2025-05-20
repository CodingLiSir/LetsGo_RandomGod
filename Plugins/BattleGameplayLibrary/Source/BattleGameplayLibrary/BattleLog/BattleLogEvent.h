
#pragma once
#include "Core/BattleLogPredefine.h"
BATTLEGAMEPLAYLIBRARY_API DECLARE_LOG_CATEGORY_EXTERN(BattleEvent, Log, All);
#define LOG_EVENT(format, ...) BATTLE_BASELOG(BattleEvent, Log, TEXT(format), ##__VA_ARGS__)
#define WARN_EVENT(format, ...) BATTLE_BASELOG(BattleEvent, Warning, TEXT(format), ##__VA_ARGS__)
#define ERROR_EVENT(format, ...) BATTLE_BASELOG(BattleEvent, Error, TEXT(format), ##__VA_ARGS__)
#define ASSERT_EVENT(condition, format, ...) ensureMsgf(condition, TEXT(format), ##__VA_ARGS__)


