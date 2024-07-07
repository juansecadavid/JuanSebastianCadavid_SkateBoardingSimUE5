// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SkateBoardGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class SKATEBOARDINGSIM_API USkateBoardGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	//USkateBoardGameInstance();

	/** Function to add points */
	UFUNCTION(BlueprintCallable, Category = "Points")
	void AddPoints(int32 Points);

	/** Function to get the current points */
	UFUNCTION(BlueprintCallable, Category = "Points")
	int32 GetPoints() const;

private:
	/** The current points */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Points", meta = (AllowPrivateAccess = "true"))
	int32 CurrentPoints = 0;
};
