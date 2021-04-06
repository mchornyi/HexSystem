// Copyright Epic Games, Inc. All Rights Reserved.

#include "HexSystemCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"

#include "HSProjectile.h"

//////////////////////////////////////////////////////////////////////////
// AHexSystemCharacter

AHexSystemCharacter::AHexSystemCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character)
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)

	//Initialize the player's Health
	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;

    //Initialize projectile class
    ProjectileClass = AHSProjectile::StaticClass( );
    //Initialize fire rate
    FireRate = 0.25f;
    bIsFiringWeapon = false;
}

//////////////////////////////////////////////////////////////////////////
// Input

void AHexSystemCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AHexSystemCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AHexSystemCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AHexSystemCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AHexSystemCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AHexSystemCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AHexSystemCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AHexSystemCharacter::OnResetVR);

    // Handle firing projectiles
    PlayerInputComponent->BindAction( "Fire", IE_Pressed, this, &AHexSystemCharacter::StartFire );
}


void AHexSystemCharacter::OnResetVR()
{
	// If HexSystem is added to a project via 'Add Feature' in the Unreal Editor the dependency on HeadMountedDisplay in HexSystem.Build.cs is not automatically propagated
	// and a linker error will result.
	// You will need to either:
	//		Add "HeadMountedDisplay" to [YourProject].Build.cs PublicDependencyModuleNames in order to build successfully (appropriate if supporting VR).
	// or:
	//		Comment or delete the call to ResetOrientationAndPosition below (appropriate if not supporting VR)
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AHexSystemCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void AHexSystemCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void AHexSystemCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AHexSystemCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AHexSystemCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AHexSystemCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get right vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

//////////////////////////////////////////////////////////////////////////
// Replicated Properties
void AHexSystemCharacter::GetLifetimeReplicatedProps( TArray <FLifetimeProperty>& OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	//Replicate current health.
	DOREPLIFETIME( AHexSystemCharacter, CurrentHealth );
}

void AHexSystemCharacter::OnHealthUpdate( )
{
	//Client-specific functionality
	if ( IsLocallyControlled( ) )
	{
		FString healthMessage = FString::Printf( TEXT( "You now have %f health remaining." ), CurrentHealth );
		GEngine->AddOnScreenDebugMessage( -1, 5.f, FColor::Blue, healthMessage );

		if ( CurrentHealth <= 0 )
		{
			FString deathMessage = FString::Printf( TEXT( "You have been killed." ) );
			GEngine->AddOnScreenDebugMessage( -1, 5.f, FColor::Red, deathMessage );
		}
	}

	//Server-specific functionality
	if ( GetLocalRole( ) == ROLE_Authority )
	{
		FString healthMessage = FString::Printf( TEXT( "%s now has %f health remaining." ), *GetFName( ).ToString( ), CurrentHealth );
		GEngine->AddOnScreenDebugMessage( -1, 5.f, FColor::Blue, healthMessage );
	}

	//Functions that occur on all machines.
	/*
		Any special functionality that should occur as a result of damage or death should be placed here.
	*/
}

void AHexSystemCharacter::OnRep_CurrentHealth( )
{
	OnHealthUpdate( );
}

void AHexSystemCharacter::SetCurrentHealth( float healthValue )
{
	if ( GetLocalRole( ) == ROLE_Authority )
	{
		CurrentHealth = FMath::Clamp( healthValue, 0.f, MaxHealth );
		OnHealthUpdate( );
	}
}

float AHexSystemCharacter::TakeDamage( float DamageTaken, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser )
{
	float damageApplied = CurrentHealth - DamageTaken;
	SetCurrentHealth( damageApplied );
	return damageApplied;
}

void AHexSystemCharacter::StartFire( )
{
    if ( !bIsFiringWeapon )
    {
        bIsFiringWeapon = true;
        UWorld* World = GetWorld( );
        World->GetTimerManager( ).SetTimer( FiringTimer, this, &AHexSystemCharacter::StopFire, FireRate, false );
        HandleFire( );
    }
}

void AHexSystemCharacter::StopFire( )
{
    bIsFiringWeapon = false;
}

void AHexSystemCharacter::HandleFire_Implementation( )
{
    FVector spawnLocation = GetActorLocation( ) + ( GetControlRotation( ).Vector( ) * 100.0f ) + ( GetActorUpVector( ) * 50.0f );
    FRotator spawnRotation = GetControlRotation( );

    FActorSpawnParameters spawnParameters;
    spawnParameters.Instigator = GetInstigator( );
    spawnParameters.Owner = this;

    AHSProjectile* spawnedProjectile = GetWorld( )->SpawnActor<AHSProjectile>( spawnLocation, spawnRotation, spawnParameters );
}