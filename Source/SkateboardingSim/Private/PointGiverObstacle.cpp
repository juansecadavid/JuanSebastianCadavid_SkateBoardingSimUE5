// Fill out your copyright notice in the Description page of Project Settings.


#include "PointGiverObstacle.h"

#include "SkateBoardGameInstance.h"
#include "Components/BoxComponent.h"
#include "SkateboardingSim/SkateboardingSimCharacter.h"

// Sets default values
APointGiverObstacle::APointGiverObstacle()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APointGiverObstacle::BeginPlay()
{
	Super::BeginPlay();
	BoxComponent = Cast<UBoxComponent>(GetDefaultSubobjectByName(TEXT("Box")));
	if (BoxComponent)
	{
		// Bind the OnOverlapBegin function to the overlap event
		BoxComponent->OnComponentBeginOverlap.AddDynamic(this, &APointGiverObstacle::OnOverlapBegin);
	}
	
}

// Called every frame
void APointGiverObstacle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APointGiverObstacle::GivePoints(ACharacter* Character)
{
	if (Character)
	{
		ASkateboardingSimCharacter* SkateboardCharacter = Cast<ASkateboardingSimCharacter>(Character);
		if (SkateboardCharacter)
		{
			USkateBoardGameInstance* GameInstance = Cast<USkateBoardGameInstance>(GetGameInstance());
			if (GameInstance)
			{
				GameInstance->AddPoints(Points);
			}
		}
	}
}

void APointGiverObstacle::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, 
								 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
								 bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor && OtherActor != this && OtherComp)
	{
		ASkateboardingSimCharacter* OverlappingCharacter = Cast<ASkateboardingSimCharacter>(OtherActor);
		if (OverlappingCharacter and OverlappingCharacter->isJumping)
		{
			// Call the function in the Character
			GivePoints(OverlappingCharacter);
			OverlappingCharacter->SlowDownTime();
		}
	}
}


