#pragma once
#include "CoreMinimal.h"

#include "Kismet/KismetSystemLibrary.h"
//#include "BattleGameplayLibrary/Template/BattleTemplate.h"
extern BATTLEGAMEPLAYLIBRARY_API TAutoConsoleVariable<int32> LogDisableCmd;


#if !NO_LOGGING

#define BATTLE_BASELOG(Arg1,Arg2,format, ...) \
{\
struct BattleLogHelper##Arg1##Arg2\
{\
	static void Print(const FString& Content,const FString& FunName)\
	{\
		if(LogDisableCmd.GetValueOnAnyThread() ==0)\
		{\
			FString print_str = FString::Printf(TEXT("<%s line:%d> Frame:%lld Client:%d %s"),*FunName,__LINE__,UKismetSystemLibrary::GetFrameCount(), GPlayInEditorID, *Content);\
			UE_LOG(Arg1, Arg2, TEXT("%s"),*print_str);\
		}\
	}\
};\
BattleLogHelper##Arg1##Arg2::Print(FString::Printf(format, ##__VA_ARGS__),FString(__FUNCTION__));\
}


#else

#define BATTLE_BASELOG(Arg1,Arg2,format, ...) \
{};\

#define BATTLE_BASELOG_THIS(Arg1,Arg2,format, ...) \
{};\

#endif


