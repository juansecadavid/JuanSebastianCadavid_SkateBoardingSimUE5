// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Sound/SoundCue.h"
#include "SkateboardingSimCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class ASkateboardingSimCharacter : public ACharacter
{
	GENERATED_BODY()

private:
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** Boost Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* BoostAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkatingTools", meta = (AllowPrivateAccess = "true") )
	UStaticMeshComponent* SkateBoard;

	/** Background music */
	UPROPERTY(EditAnywhere, Category = "Audio")
	USoundBase* BackgroundMusic;

	/** Audio Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio", meta = (AllowPrivateAccess = "true"))
	class UAudioComponent* AudioComponent;

	/** Timer handle for slow motion effect */
	FTimerHandle TimerHandle_SlowMotion;

	/** Variables para almacenar los valores de movimiento */
	float MoveForwardValue;
	float MoveRightValue;
	float CurrentForwardSpeed;
	float MF_Value = 0.0f;
	float TargetSkateVelocity = 0.0f;

protected:
	/** Called when the game starts or when spawned */
	virtual void BeginPlay() override;

	/** Called every frame */
	virtual void Tick(float DeltaSeconds) override;

	/** Set up player input component */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Handles starting movement input */
	void StartMove(const FInputActionValue& Value);

	/** Handles starting jump input */
	void StartJumping();
	
	/** Handles movement input */
	void Move(const FInputActionValue& Value);

	/** Handles looking input */
	void Look(const FInputActionValue& Value);

	/** Handles stopping movement input */
	void StopMove();

	/** Adjusts the character's rotation to match the slope */
	void AdjustRotationToSlope(float DeltaTime);

	/** Restores normal speed after slow motion */
	void RestoreNormalSpeed();
	
	/** Applies sliding force when on a slope */
	void ApplySlidingForce();

public:
	ASkateboardingSimCharacter();

	/** Returns CameraBoom subobject */
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject */
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/** Returns user inputs */
	UFUNCTION(BlueprintCallable, Category = "Input")
	FVector2D GetUserMyInputs() const;

	/** Activates slow down time effect */
	UFUNCTION(BlueprintCallable, Category = "SlowDown")
	void SlowDownTime();

	/** Indicates if the character is boosting */
	UPROPERTY(BlueprintReadOnly)
	bool isBoosting = false;

	/** Indicates if the character is jumping */
	UPROPERTY(BlueprintReadWrite)
	bool isJumping = false;
};

