// Copyright Epic Games, Inc. All Rights Reserved.

#include "SkateboardingSimCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ASkateboardingSimCharacter

ASkateboardingSimCharacter::ASkateboardingSimCharacter()
{
	//CustomRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("CustomRootComponent"));
	//RootComponent = CustomRootComponent;

	// Mover el CapsuleComponent para que ya no sea el componente raíz
	//GetCapsuleComponent()->SetupAttachment(CustomRootComponent);
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void ASkateboardingSimCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	SkateBoard = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("SkateStaticMesh")));
	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	// Obtener la inclinación del plano
	FVector FloorNormal = GetCharacterMovement()->CurrentFloor.HitResult.ImpactNormal;
	float FloorAngle = FMath::RadiansToDegrees(FMath::Acos(FloorNormal.Z));

	// Aplicar fuerza adicional si el personaje está en una pendiente
	if (FloorAngle > 0)
	{
		FVector SlideDirection = FVector(FloorNormal.X, FloorNormal.Y, 0.0f).GetSafeNormal();
		FVector Force = SlideDirection * 2000.0f; // Ajusta este valor según sea necesario
		GetCharacterMovement()->AddForce(Force);
	}
}
void ASkateboardingSimCharacter::Tick(float DeltaSeconds)
{
	const FVector ForwardDirection = SkateBoard->GetRightVector();
	MF_Value = FMath::Lerp(MF_Value, TargetSkateVelocity, 0.01f);

	AddMovementInput(ForwardDirection, MF_Value);
	ApplySlidingForce();
	AdjustRotationToSlope(DeltaSeconds);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow,FString::Printf( TEXT("Supone que hay pendiente: %f"),GetCharacterMovement()->GroundFriction));
}

//////////////////////////////////////////////////////////////////////////
// Input

void ASkateboardingSimCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ASkateboardingSimCharacter::StartJumping);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Started, this, &ASkateboardingSimCharacter::StartMove);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASkateboardingSimCharacter::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ASkateboardingSimCharacter::StopMove);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASkateboardingSimCharacter::Look);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ASkateboardingSimCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector RightDirection = SkateBoard->GetForwardVector();

		TargetSkateVelocity = MovementVector.Y;
        

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("MoveForwardValue: %f"), MF_Value));
		// add movement 
		AddMovementInput(RightDirection, MovementVector.X*0.02f);

		MoveForwardValue = MovementVector.Y;
		MoveRightValue = MovementVector.X;
	}
}

void ASkateboardingSimCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

FVector2D ASkateboardingSimCharacter::GetUserMyInputs() const
{
	return FVector2D(MoveForwardValue, MoveRightValue);
}

void ASkateboardingSimCharacter::StartMove(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	UE_LOG(LogTemplateCharacter, Log, TEXT("Move input started"));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Move input started"));
	if(MovementVector.Y!=0)
	{
		if(MovementVector.Y<0)
			isBoosting = true;
		GetCharacterMovement()->BrakingDecelerationWalking = 3000.f;
		GetCharacterMovement()->GroundFriction = 8.0f;
	}
}

void ASkateboardingSimCharacter::StopMove()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Move input finished"));
	TargetSkateVelocity = 0.0f;
	isBoosting = false;
	//MF_Value = 0.0f;
}
void ASkateboardingSimCharacter::StartJumping()
{
	isJumping = false;
	isJumping = true;
	
}

void ASkateboardingSimCharacter::SlowDownTime()
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.3f);
	GetWorldTimerManager().SetTimer(TimerHandle_SlowMotion, this, &ASkateboardingSimCharacter::RestoreNormalSpeed, 0.5f, false);
}


void ASkateboardingSimCharacter::RestoreNormalSpeed()
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
}

void ASkateboardingSimCharacter::ApplySlidingForce()
{
	if (!isBoosting) // Solo aplicar la fuerza de deslizamiento si no se están presionando los botones de movimiento
	{
		FVector FloorNormal = GetCharacterMovement()->CurrentFloor.HitResult.ImpactNormal;
        
		// Si la pendiente es suficientemente inclinada, aplicar fuerza de deslizamiento
		if (FloorNormal.Z < 1.0f)  // 1.0f significa completamente plano, menor es una pendiente
		{
			GetCharacterMovement()->BrakingDecelerationWalking = 100.f;
			GetCharacterMovement()->GroundFriction = 1.0f;
			FVector Gravity = FVector(0.0f, 0.0f, -980.0f);  // Gravedad estándar en Unreal Engine
			FVector GravityParallel = Gravity - (FloorNormal * FVector::DotProduct(Gravity, FloorNormal));  // Componente paralela de la gravedad
			FVector SlideDirection = GravityParallel.GetSafeNormal();

			// Aplicar fuerza de deslizamiento
			GetCharacterMovement()->AddForce(SlideDirection * GetCharacterMovement()->Mass * 300.0f);  // Ajusta el multiplicador según sea necesario
		}
	}
}

void ASkateboardingSimCharacter::AdjustRotationToSlope(float DeltaTime)
{
	// Obtener la normal del suelo
	FVector FloorNormal = GetCharacterMovement()->CurrentFloor.HitResult.ImpactNormal;

	// Si la normal del suelo es válida
	if (FloorNormal.Z < 1.0f)
	{
		// Calcular la nueva rotación basada en la normal del suelo
		FRotator TargetSlopeRotation = FRotationMatrix::MakeFromZX(FloorNormal, GetActorForwardVector()).Rotator();

		// Determinar la dirección del movimiento
		FVector ForwardVector = GetActorForwardVector();
		FVector MovementDirection = GetVelocity().GetSafeNormal();

		// Proyectar el vector de movimiento en el plano de la pendiente
		FVector ProjectedMovement = FVector::VectorPlaneProject(MovementDirection, FloorNormal);

		// Determinar si el personaje está subiendo o bajando
		float DotProduct = FVector::DotProduct(ForwardVector, ProjectedMovement);
		bool bIsMovingUpSlope = DotProduct > 0;

		// Agregar un offset adicional a la rotación objetivo
		float RotationOffsetPitch = 130.0f; // Ajusta este valor para cambiar el offset en el eje de inclinación (pitch)

		if (bIsMovingUpSlope)
		{
			TargetSlopeRotation.Pitch += RotationOffsetPitch;
		}
		else
		{
			TargetSlopeRotation.Pitch -= RotationOffsetPitch;
		}

		// Interpolar entre la rotación actual y la rotación objetivo
		FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetSlopeRotation, DeltaTime, 5.0f); // Ajusta el valor 5.0f para cambiar la velocidad de la interpolación

		// Aplicar la nueva rotación al personaje
		SetActorRotation(NewRotation);
	}
}



