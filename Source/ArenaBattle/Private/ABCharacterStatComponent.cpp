// Fill out your copyright notice in the Description page of Project Settings.


#include "ABCharacterStatComponent.h"
#include "ABGameInstance.h"
// Sets default values for this component's properties
UABCharacterStatComponent::UABCharacterStatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;

	// ...
	Level = 1;
}


// Called when the game starts
void UABCharacterStatComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UABCharacterStatComponent::InitializeComponent()
{
	Super::InitializeComponent();

	SetNewLevel(5);
}


// Called every frame
void UABCharacterStatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UABCharacterStatComponent::SetNewLevel(int32 NewLevel)
{
	auto GameInstance = Cast<UABGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));

	if(GameInstance != nullptr)
	{
		CurrentStatData = GameInstance->GetABCharacterData(NewLevel);
		if(CurrentStatData != nullptr)
		{
			Level = NewLevel;
			SetHP(CurrentStatData->MaxHP);
		}
		else
		{
			ABLOG(Error, TEXT("Level (%d) data doesn't exist"), NewLevel);
		}
	}
}

void UABCharacterStatComponent::SetDamage(float Damage)
{
	SetHP(FMath::Clamp<float>(CurrentHP - Damage, 0.0f, CurrentStatData->MaxHP));
	ABLOG(Warning, TEXT("Current HP : %f"), CurrentHP);
	if(CurrentHP <= 0.0f)
	{
		OnHPIsZero.Broadcast();
	}
}

void UABCharacterStatComponent::SetHP(float NewHP)
{
	CurrentHP = NewHP;
	OnHPChange.Broadcast();
	if(CurrentHP < KINDA_SMALL_NUMBER)
	{
		CurrentHP = 0.0f;
		OnHPIsZero.Broadcast();
	}
}

float UABCharacterStatComponent::GetAttack()
{
	return CurrentStatData->Attack;
}

float UABCharacterStatComponent::GetHPRatio()
{
	return (CurrentHP < KINDA_SMALL_NUMBER) ? 0.0f : (CurrentHP / CurrentStatData->MaxHP);
}


