// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ScoreObject.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "PointGiverObstacle.generated.h"

UCLASS()
class SKATEBOARDINGSIM_API APointGiverObstacle : public AActor, public  IScoreObject
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APointGiverObstacle();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Pointer to the Box Component
	UBoxComponent* BoxComponent;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void GivePoints(ACharacter* Character) override;

	UPROPERTY(EditAnywhere, Category = "Points")
	int32 Points = 10;  // Example points

	// Function to handle overlap
	UFUNCTION()
	void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, 
						class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
						bool bFromSweep, const FHitResult& SweepResult);
};
