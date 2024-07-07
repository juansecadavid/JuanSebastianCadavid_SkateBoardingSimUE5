// Fill out your copyright notice in the Description page of Project Settings.


#include "SkateBoardGameInstance.h"

void USkateBoardGameInstance::AddPoints(int32 Points)
{
	CurrentPoints += Points;
}

int32 USkateBoardGameInstance::GetPoints() const
{
	return CurrentPoints;
}
