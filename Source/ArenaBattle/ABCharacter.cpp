// Fill out your copyright notice in the Description page of Project Settings.


#include "ABCharacter.h"
#include "ABAnimInstance.h"
#include "ABWeapon.h"

// Sets default values
AABCharacter::AABCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SPRINGARM"));
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("CAMERA"));

	SpringArm->SetupAttachment(GetCapsuleComponent());
	Camera->SetupAttachment(SpringArm);

	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -88.0f),
		FRotator(0.0f, -90.0f, 0.0f));

	SpringArm->TargetArmLength = 400.f;
	SpringArm->SetRelativeLocation(FVector(-15.0f, 0.0f, 0.0f));
	ArmLengthSpeed = 3.0f;
	ArmRotationSpeed = 10.0f;

	GetCharacterMovement()->JumpZVelocity = 800.0f;
	
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> SK_CARDBOARD(TEXT("/Game/InfinityBladeWarriors/Character/CompleteCharacters/SK_CharM_Cardboard.SK_CharM_Cardboard"));
	if (SK_CARDBOARD.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(SK_CARDBOARD.Object);
		ABLOG(Warning, TEXT("Successfully loaded Cardboard model"));
	}
    
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
	
	static ConstructorHelpers::FClassFinder<UAnimInstance> WARRIOR_ANIM
	(TEXT("/Game/Animation/WarriorAnimBlueprint.WarriorAnimBlueprint_C"));
	if (WARRIOR_ANIM.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(WARRIOR_ANIM.Class);
	}

	SetControlMode(EControlMode::DIABLO);

	IsAttacking = false;

	// Combo Related
	MaxCombo = 4;
	AttackEndComboState();

	GetCapsuleComponent()->SetCollisionProfileName(TEXT("ABCharacter"));
}

// Called when the game starts or when spawned
void AABCharacter::BeginPlay()
{
	Super::BeginPlay();

	FName WeaponSocket(TEXT("hand_rSocket"));
	auto CurWeapon = GetWorld()->SpawnActor<AABWeapon>(FVector::ZeroVector,
		FRotator::ZeroRotator);

	if(CurWeapon != nullptr)
	{
		CurWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocket);
	}
}

void AABCharacter::SetControlMode(EControlMode ControlMode)
{
	CurrentControlMode = ControlMode;
	
	switch (ControlMode)
	{
		case EControlMode::GTA:
			ArmLengthTo = 450.0f;
			SpringArm->bUsePawnControlRotation = true;
			SpringArm->bInheritPitch = true;
			SpringArm->bInheritRoll = true;
			SpringArm->bInheritYaw = true;
			SpringArm->bDoCollisionTest = true;
			bUseControllerRotationYaw = false;
			GetCharacterMovement()->bOrientRotationToMovement = true;
			GetCharacterMovement()->bUseControllerDesiredRotation = false;
			GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
			break;

		case EControlMode::DIABLO:
			ArmLengthTo = 800.0f;
			ArmRotationTo = FRotator(-45.0f, 0.0f, 0.0f);
			SpringArm->bUsePawnControlRotation = false;
			SpringArm->bInheritPitch = false;
			SpringArm->bInheritRoll = false;
			SpringArm->bInheritYaw = false;
			SpringArm->bDoCollisionTest = false;
			bUseControllerRotationYaw = false;
			GetCharacterMovement()->bOrientRotationToMovement = false;
			GetCharacterMovement()->bUseControllerDesiredRotation = true;
			GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
			break;
	}
}

// Called every frame
void AABCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, ArmLengthTo, DeltaTime, ArmLengthSpeed);
	
	switch (CurrentControlMode)
	{
	case EControlMode::DIABLO:
		SpringArm->SetRelativeRotation(FMath::RInterpTo(SpringArm->GetRelativeRotation(), ArmRotationTo, DeltaTime, ArmRotationSpeed));
		GetController()->SetControlRotation(FRotationMatrix::MakeFromX(DirectionToMove).Rotator());
		AddMovementInput(DirectionToMove);
		break;
	}
}

void AABCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ABAnim = Cast<UABAnimInstance>(GetMesh()->GetAnimInstance());

	ABAnim->OnMontageEnded.AddDynamic(this, &AABCharacter::OnAttackMontageEnded);
	ABAnim->OnNextAttackCheck.AddLambda([this]()-> void
	{
		ABLOG(Warning, TEXT("OnNextAttackCheck"));
		CanNextCombo = false;

		if(IsComboInputOn)
		{
			AttackStartComboState();
			ABAnim->JumpToAttackMontageSection(CurrentCombo);
		}
	});
	ABAnim->OnAttackHitCheck.AddUObject(this, &AABCharacter::AttackCheck);

}

float AABCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator,
	AActor* DamageCauser)
{
	float FinalDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	ABLOG(Warning, TEXT("Actor : %s took Damage : %f"), *GetName(), FinalDamage);

	if(FinalDamage > 0.0f)
	{
		ABAnim->SetDeadAnim();
		SetActorEnableCollision(false);
	}
	
	return FinalDamage;
}

// Called to bind functionality to input
void AABCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);  

	PlayerInputComponent->BindAxis(TEXT("UpDown"), this, &AABCharacter::UpDown);
	PlayerInputComponent->BindAxis(TEXT("LeftRight"), this, &AABCharacter::LeftRight);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AABCharacter::LookUp);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AABCharacter::Turn);
	PlayerInputComponent->BindAction(TEXT("ViewChange"), EInputEvent::IE_Pressed, this, &AABCharacter::ViewChange);
	PlayerInputComponent->BindAction(TEXT("Jump"), EInputEvent::IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Attack"), EInputEvent::IE_Pressed, this, &AABCharacter::Attack);

	bUseControllerRotationYaw = false;
}

void AABCharacter::UpDown(float NewAxisValue)
{
	switch(CurrentControlMode)
	{
	case EControlMode::GTA:
		AddMovementInput(FRotationMatrix(FRotator(0.0f,GetControlRotation().Yaw, 0.0f)).GetUnitAxis(EAxis::X), NewAxisValue);
		break;

	case EControlMode::DIABLO:
		DirectionToMove.X = NewAxisValue;
		break;
	}
	
}
void AABCharacter::LeftRight(float NewAxisValue)
{
	switch (CurrentControlMode)
	{
	case EControlMode::GTA:
		AddMovementInput(FRotationMatrix(FRotator(0.0f, GetControlRotation().Yaw, 0.0f)).GetUnitAxis(EAxis::Y), NewAxisValue);
		break;
	case EControlMode::DIABLO:
		DirectionToMove.Y = NewAxisValue;
		break;
	}
	
}
void AABCharacter::LookUp(float NewAxisValue)
{
	switch (CurrentControlMode)
	{
	case EControlMode::GTA:
		AddControllerPitchInput(NewAxisValue);
		break;

	case EControlMode::DIABLO:
		break;
	}
}
void AABCharacter::Turn(float NewAxisValue)
{
	switch (CurrentControlMode)
	{
	case EControlMode::GTA:
		AddControllerYawInput(NewAxisValue);
		break;
	case EControlMode::DIABLO:
		break;
	}
}

void AABCharacter::ViewChange()
{
	switch (CurrentControlMode)
	{
	case EControlMode::GTA:
		GetController()->SetControlRotation(GetActorRotation());
		SetControlMode(EControlMode::DIABLO);
		break;
	case EControlMode::DIABLO:
		GetController()->SetControlRotation(SpringArm->GetRelativeRotation());
		SetControlMode(EControlMode::GTA);
		break;
	}
}

void AABCharacter::Attack()
{
	if (IsAttacking)
	{
		if (CanNextCombo) IsComboInputOn = true;
	}

	else
	{
		AttackStartComboState();
		ABAnim->PlayAttackMontage();
		ABAnim->JumpToAttackMontageSection(CurrentCombo);
		IsAttacking = true;
	}
}

inline void AABCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	IsAttacking = false;
	AttackEndComboState();
}

void AABCharacter::AttackStartComboState()
{
	CanNextCombo = true;
	IsComboInputOn = false;
	CurrentCombo = FMath::Clamp<int32>(CurrentCombo + 1, 1, MaxCombo);
}

void AABCharacter::AttackEndComboState()
{
	IsComboInputOn = false;
	CanNextCombo = false;
	CurrentCombo = 0;
}

void AABCharacter::AttackCheck()
{
	FHitResult HitResult;
	FCollisionQueryParams Params(NAME_None, false, this);
	bool bResult = GetWorld()->SweepSingleByChannel(
		HitResult,
		GetActorLocation(), GetActorLocation() + GetActorForwardVector() * 200.0f,
		FQuat::Identity,
		ECollisionChannel::ECC_GameTraceChannel2,
		FCollisionShape::MakeSphere(50.0f),
		Params);

		if(bResult)
		{
			if(HitResult.Actor.IsValid())
			{
				ABLOG(Warning, TEXT("Hit Actor Name: %s"), *HitResult.Actor->GetName());

				FDamageEvent DamageEvent;
				HitResult.Actor->TakeDamage(50.0f, DamageEvent, GetController(), this);
			}
		}
}

