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


// Initializes the character with default settings for movement and camera components.
ASkateboardingSimCharacter::ASkateboardingSimCharacter()
{
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
        
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

    GetCharacterMovement()->JumpZVelocity = 700.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = 500.f;
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
    GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f;
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
}

// Called when the game starts or when spawned. Initializes the SkateBoard component and sets up the input mapping context.
void ASkateboardingSimCharacter::BeginPlay()
{
    Super::BeginPlay();

    SkateBoard = Cast<UStaticMeshComponent>(GetDefaultSubobjectByName(TEXT("SkateStaticMesh")));
    
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }
}

//Called every frame. Updates movement input, applies sliding force, and adjusts the character's rotation to match the slope.
void ASkateboardingSimCharacter::Tick(float DeltaSeconds)
{
    const FVector ForwardDirection = SkateBoard->GetRightVector();
    MF_Value = FMath::Lerp(MF_Value, TargetSkateVelocity, 0.01f);
    MF_Value = FMath::Clamp(MF_Value, -FLT_MAX, 0.0f);
    
    AddMovementInput(ForwardDirection, MF_Value);
    ApplySlidingForce();
    AdjustRotationToSlope(DeltaSeconds);
}

//////////////////////////////////////////////////////////////////////////
// Input
//Binds the input actions for jumping, moving, and looking to their corresponding functions.
void ASkateboardingSimCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) 
    {
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ASkateboardingSimCharacter::StartJumping);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Started, this, &ASkateboardingSimCharacter::StartMove);
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASkateboardingSimCharacter::Move);
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ASkateboardingSimCharacter::StopMove);

        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASkateboardingSimCharacter::Look);
    }
}

//Handles the movement input from the player, setting the target velocity and direction based on input.
void ASkateboardingSimCharacter::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        const FVector RightDirection = SkateBoard->GetForwardVector();

        TargetSkateVelocity = MovementVector.Y;

        AddMovementInput(RightDirection, MovementVector.X * 0.02f);
        
        MoveForwardValue = MovementVector.Y;
        MoveRightValue = MovementVector.X;
    }
}

//Handles the look input from the player, allowing the character to rotate based on mouse or controller input.
void ASkateboardingSimCharacter::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(LookAxisVector.Y);
    }
}

//Returns the current movement input values for forward and right directions.
FVector2D ASkateboardingSimCharacter::GetUserMyInputs() const
{
    return FVector2D(MoveForwardValue, MoveRightValue);
}

//Called when movement input starts. Adjusts character movement settings based on input direction.
void ASkateboardingSimCharacter::StartMove(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();
    if (MovementVector.Y != 0)
    {
        if (MovementVector.Y < 0)
            isBoosting = true;
        GetCharacterMovement()->BrakingDecelerationWalking = 3000.f;
        GetCharacterMovement()->GroundFriction = 8.0f;
    }
}

//Called when movement input stops. Resets the target velocity and boosting state.
void ASkateboardingSimCharacter::StopMove()
{
    TargetSkateVelocity = 0.0f;
    isBoosting = false;
}

//Sets the jumping state to true when the jump input is received.
void ASkateboardingSimCharacter::StartJumping()
{
    isJumping = true;
}

//Activates slow motion by reducing the global time dilation for a short duration.
void ASkateboardingSimCharacter::SlowDownTime()
{
    UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.3f);
    GetWorldTimerManager().SetTimer(TimerHandle_SlowMotion, this, &ASkateboardingSimCharacter::RestoreNormalSpeed, 0.5f, false);
}

//Restores the global time dilation to normal speed after slow motion ends.
void ASkateboardingSimCharacter::RestoreNormalSpeed()
{
    UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
}

//Applies a sliding force to the character when on a slope, simulating the effect of gravity on an incline.
void ASkateboardingSimCharacter::ApplySlidingForce()
{
    if (!isBoosting)
    {
        FVector FloorNormal = GetCharacterMovement()->CurrentFloor.HitResult.ImpactNormal;

        if (FloorNormal.Z < 1.0f)
        {
            GetCharacterMovement()->BrakingDecelerationWalking = 100.f;
            GetCharacterMovement()->GroundFriction = 1.0f;

            FVector Gravity = FVector(0.0f, 0.0f, -980.0f);
            FVector GravityParallel = Gravity - (FloorNormal * FVector::DotProduct(Gravity, FloorNormal));
            FVector SlideDirection = GravityParallel.GetSafeNormal();

            GetCharacterMovement()->AddForce(SlideDirection * GetCharacterMovement()->Mass * 300.0f);
        }
    }
}

//Adjusts the character's rotation to align with the slope of the surface they are moving on.
void ASkateboardingSimCharacter::AdjustRotationToSlope(float DeltaTime)
{
    FVector FloorNormal = GetCharacterMovement()->CurrentFloor.HitResult.ImpactNormal;

    if (FloorNormal.Z < 1.0f)
    {
        FRotator TargetSlopeRotation = FRotationMatrix::MakeFromZX(FloorNormal, GetActorForwardVector()).Rotator();

        FVector ForwardVector = GetActorForwardVector();
        FVector MovementDirection = GetVelocity().GetSafeNormal();
        FVector ProjectedMovement = FVector::VectorPlaneProject(MovementDirection, FloorNormal);

        float DotProduct = FVector::DotProduct(ForwardVector, ProjectedMovement);
        bool bIsMovingUpSlope = DotProduct > 0;

        float RotationOffsetPitch = 150.0f;

        if (bIsMovingUpSlope)
        {
            TargetSlopeRotation.Pitch += RotationOffsetPitch;
        }
        else
        {
            TargetSlopeRotation.Pitch -= RotationOffsetPitch;
        }

        FRotator NewRotation = FMath::RInterpTo(GetActorRotation(), TargetSlopeRotation, DeltaTime, 2.5f);

        SetActorRotation(NewRotation);
    }
}
