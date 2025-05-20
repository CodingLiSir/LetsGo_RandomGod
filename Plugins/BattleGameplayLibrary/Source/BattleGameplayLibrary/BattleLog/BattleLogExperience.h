
#pragma once
#include "Core/BattleLogPredefine.h"
BATTLEGAMEPLAYLIBRARY_API DECLARE_LOG_CATEGORY_EXTERN(BattleLogExperience, Log, All);
#define LOG_Experience(format, ...) BATTLE_BASELOG(BattleLogExperience, Log, TEXT(format), ##__VA_ARGS__)
#define WARN_Experience(format, ...) BATTLE_BASELOG(BattleLogExperience, Warning, TEXT(format), ##__VA_ARGS__)
#define ERROR_Experience(format, ...) BATTLE_BASELOG(BattleLogExperience, Error, TEXT(format), ##__VA_ARGS__)
#define ASSERT_Experience(condition, format, ...) ensureMsgf(condition, TEXT(format), ##__VA_ARGS__)


