// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RGPawnData.generated.h"

/**
 * 
 */
UCLASS()
class RANDOMGOD_API URGPawnData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	
	URGPawnData(const FObjectInitializer& ObjectInitializer);

public:
	// Class to instantiate for this pawn (should usually derive from ALyraPawn or ALyraCharacter).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RG|Pawn")
	TSubclassOf<APawn> PawnClass;
	
};
