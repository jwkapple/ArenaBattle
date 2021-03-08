// Fill out your copyright notice in the Description page of Project Settings.


#include "ABCharacter.h"
#include "ABAnimInstance.h"
#include "ABCharacterStatComponent.h"
#include "ABWeapon.h"
#include "ABCharacterWidget.h"
#include "Components/WidgetComponent.h"
#include "ABPlayerController.h"
#include "ABAIController.h"
#include "ABCharacterSetting.h"
#include "ABGameInstance.h"
#include "ABPlayerState.h"
#include "ABHUDWidget.h"

// Sets default values
AABCharacter::AABCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GetMesh()->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -88.0f),
		FRotator(0.0f, -90.0f, 0.0f));

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SPRINGARM"));
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("CAMERA"));

	SpringArm->TargetArmLength = 400.f;
	SpringArm->SetRelativeLocation(FVector(-15.0f, 0.0f, 0.0f));
	ArmLengthSpeed = 3.0f;
	ArmRotationSpeed = 10.0f;

    CharacterStat = CreateDefaultSubobject<UABCharacterStatComponent>(TEXT("CHARACTERSTAT"));
    HPBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBARWIDGET"));

	SpringArm->SetupAttachment(GetCapsuleComponent());
	Camera->SetupAttachment(SpringArm);
	HPBarWidget->SetupAttachment(GetMesh());

	HPBarWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 180.0f));
	HPBarWidget->SetWidgetSpace(EWidgetSpace::Screen);
	static ConstructorHelpers::FClassFinder<UUserWidget> UI_HUD(TEXT("/Game/UI/UI_HPBar.UI_HPBar_C"));
	if(UI_HUD.Succeeded())
	{
		HPBarWidget->SetWidgetClass(UI_HUD.Class);
		HPBarWidget->SetDrawSize(FVector2D(150.0f, 50.0f));
	}
	
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

	AIControllerClass = AABAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	AssetIdx = 4;
	DeadTimer = 5.0f;
	SetActorHiddenInGame(true);
	HPBarWidget->SetHiddenInGame(true);
}

// Called when the game starts or when spawned
void AABCharacter::BeginPlay()
{
	Super::BeginPlay();

	IsPlayer = IsPlayerControlled();

	if (IsPlayer) ABPlayerController = Cast<AABPlayerController>(GetController());
	else ABAIController = Cast<AABAIController>(GetController());
	auto CharacterWidget = Cast<UABCharacterWidget>(HPBarWidget->GetUserWidgetObject());
	if (CharacterWidget != nullptr) 
		CharacterWidget->BindCharacterStatComponent(CharacterStat);

	auto DefaultSetting = GetDefault<UABCharacterSetting>();

	AssetIdx = IsPlayer ? 4 : FMath::RandRange(0, DefaultSetting->CharacterAssets.Num() - 1);

	CharacterAssetToLoad = DefaultSetting->CharacterAssets[AssetIdx];
	auto GameInstance = Cast<UABGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));

	AssetStreamingHandle = GameInstance->StreamableManager.RequestAsyncLoad(CharacterAssetToLoad, FStreamableDelegate::CreateUObject(this, &AABCharacter::OnAssetLoadCompleted));
	SetCharacterState(ECharacterState::LOADING);
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

	CharacterStat->OnHPIsZero.AddLambda([this]()-> void
	{
		ABLOG(Warning, TEXT("OnHPIsZero"));
		ABAnim->SetDeadAnim();
		
		SetActorEnableCollision(false);
	});
}

float AABCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator,
	AActor* DamageCauser)
{
	float FinalDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	ABLOG(Warning, TEXT("Actor : %s took Damage : %f"), *GetName(), FinalDamage);

	CharacterStat->SetDamage(FinalDamage);
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
bool AABCharacter::CanSetWeapon()
{
	return CurrentWeapon == nullptr;
}
void AABCharacter::SetWeapon(AABWeapon* NewWeapon)
{
	FName WeaponSocket(TEXT("hand_rSocket"));

	if(NewWeapon != nullptr)
	{
		NewWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponSocket);
		NewWeapon->SetOwner(this);
		CurrentWeapon = NewWeapon;
	}
}

void AABCharacter::OnAssetLoadCompleted()
{
	USkeletalMesh* AssetLoaded = Cast<USkeletalMesh>(AssetStreamingHandle->GetLoadedAsset());
	AssetStreamingHandle.Reset();
	if (AssetLoaded != nullptr)
	{
		GetMesh()->SetSkeletalMesh(AssetLoaded);
		SetCharacterState(ECharacterState::READY);
	}
}

void AABCharacter::SetCharacterState(ECharacterState NewState)
{
	CurrentState = NewState;
	switch (NewState)
	{
		case ECharacterState::PREINIT:
		{

			break;
		}
		case ECharacterState::LOADING:
		{
			ABLOG(Warning, TEXT("Current State: LOADING"));
			if(IsPlayer) 
			{
				DisableInput(ABPlayerController);
				auto ABPlayerState = Cast<AABPlayerState>(GetController()->PlayerState);
				CharacterStat->SetNewLevel(ABPlayerState->GetCharacterLevel());

				ABPlayerController->GetHUDWidget()->BindCharacterStat(CharacterStat);
			}
			SetActorHiddenInGame(true);
			HPBarWidget->SetHiddenInGame(true);

			break;
		}
		case ECharacterState::READY:
		{
			ABLOG(Warning, TEXT("Current State: READY"));
			SetActorHiddenInGame(false);
			HPBarWidget->SetHiddenInGame(false);

			auto CharacterWidget = Cast<UABCharacterWidget>(HPBarWidget->GetUserWidgetObject());
			CharacterWidget->BindCharacterStatComponent(CharacterStat);
			CharacterStat->OnHPIsZero.AddLambda([this]() -> void {
				SetCharacterState(ECharacterState::DEAD);
			});

			if (IsPlayer)
			{
				SetControlMode(EControlMode::DIABLO);
				GetCharacterMovement()->MaxWalkSpeed = 600.0f;
				EnableInput(ABPlayerController);
			}
			else
			{
				GetCharacterMovement()->MaxWalkSpeed = 400.0f;
				ABAIController->RunAI();
			}
			break;
		}
		case ECharacterState::DEAD:
		{
			ABLOG(Warning, TEXT("Current State: DEAD"));
			SetActorEnableCollision(false);
			GetMesh()->SetHiddenInGame(true);
			HPBarWidget->SetHiddenInGame(true);
			ABAnim->SetDeadAnim();

			if (IsPlayer) DisableInput(ABPlayerController);
			else 
			{
				ABAIController->StopAI();
			
			}

			GetWorld()->GetTimerManager().SetTimer(DeadTimerHandle, FTimerDelegate::CreateLambda([this]() -> void {
				if (IsPlayer) ABPlayerController->RestartLevel();
			}), DeadTimer, false);
			break;
		}
	}
}
ECharacterState AABCharacter::GetCharacterState() const
{
	return CurrentState;
}

inline void AABCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	IsAttacking = false;
	AttackEndComboState();
	OnAttackEnd.Broadcast();
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
				HitResult.Actor->TakeDamage(CharacterStat->GetAttack(), DamageEvent, GetController(), this);
			}
		}
}

