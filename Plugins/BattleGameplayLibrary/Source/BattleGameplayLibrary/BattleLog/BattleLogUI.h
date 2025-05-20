
#pragma once
#include "Core/BattleLogPredefine.h"
BATTLEGAMEPLAYLIBRARY_API DECLARE_LOG_CATEGORY_EXTERN(BattleUI, Log, All);
#define LOG_UI(format, ...) BATTLE_BASELOG(BattleUI, Log, TEXT(format), ##__VA_ARGS__)
#define WARN_UI(format, ...) BATTLE_BASELOG(BattleUI, Warning, TEXT(format), ##__VA_ARGS__)
#define ERROR_UI(format, ...) BATTLE_BASELOG(BattleUI, Error, TEXT(format), ##__VA_ARGS__)
#define ASSERT_UI(condition, format, ...) ensureMsgf(condition, TEXT(format), ##__VA_ARGS__)


