
#pragma once
#include "Core/BattleLogPredefine.h"
BATTLEGAMEPLAYLIBRARY_API DECLARE_LOG_CATEGORY_EXTERN(BattleGamePlay, Log, All);
#define LOG_GAMEPLAY(format, ...) BATTLE_BASELOG(BattleGamePlay, Log, TEXT(format), ##__VA_ARGS__)
#define WARN_GAMEPLAY(format, ...) BATTLE_BASELOG(BattleGamePlay, Warning, TEXT(format), ##__VA_ARGS__)
#define ERROR_GAMEPLAY(format, ...) BATTLE_BASELOG(BattleGamePlay, Error, TEXT(format), ##__VA_ARGS__)
#define ASSERT_GAMEPLAY(condition, format, ...) ensureMsgf(condition, TEXT(format), ##__VA_ARGS__)


