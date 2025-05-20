
#pragma once
#include "Core/BattleLogPredefine.h"
BATTLEGAMEPLAYLIBRARY_API DECLARE_LOG_CATEGORY_EXTERN(BattleTest, Log, All);
#define LOG_TEST(format, ...) BATTLE_BASELOG(BattleTest, Log, TEXT(format), ##__VA_ARGS__)
#define WARN_TEST(format, ...) BATTLE_BASELOG(BattleTest, Warning, TEXT(format), ##__VA_ARGS__)
#define ERROR_TEST(format, ...) BATTLE_BASELOG(BattleTest, Error, TEXT(format), ##__VA_ARGS__)
#define ASSERT_TEST(condition, format, ...) ensureMsgf(condition, TEXT(format), ##__VA_ARGS__)


