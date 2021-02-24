// Fill out your copyright notice in the Description page of Project Settings.


#include "ABAnimInstance.h"

UABAnimInstance::UABAnimInstance()
{
	CurrentPawnSpeed = 0.0f;
	IsInAir = false;

	static ConstructorHelpers::FObjectFinder<UAnimMontage> ATTACK_MONTAGE(TEXT("/Game/Animation/SK_Mannequin_Skeleton_Montage.SK_Mannequin_Skeleton_Montage"));
	if(ATTACK_MONTAGE.Succeeded())
	{
		AttackMontage = ATTACK_MONTAGE.Object;
	}

	IsDead = false;
}

void UABAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	auto Pawn = TryGetPawnOwner();
	if (!::IsValid(Pawn)) return;

	if(!IsDead)
	{
		CurrentPawnSpeed = Pawn->GetVelocity().Size();

		auto Character = Cast<ACharacter>(Pawn);
		if(Character)
		{
			IsInAir = Character->GetCharacterMovement()->IsFalling();
		}
	}
}

void UABAnimInstance::PlayAttackMontage()
{
	Montage_Play(AttackMontage, 1.0f);
}

void UABAnimInstance::JumpToAttackMontageSection(int32 NewSection)
{
	UE_LOG(ArenaBattle, Warning, TEXT("%d"), NewSection);
	
	Montage_JumpToSection(GetAttackMontageSectionName(NewSection), AttackMontage);
}

void UABAnimInstance::AnimNotify_AttackHitCheck()
{
	OnAttackHitCheck.Broadcast();
}

void UABAnimInstance::AnimNotify_NextAttackCheck()
{
	OnNextAttackCheck.Broadcast();
}

FName UABAnimInstance::GetAttackMontageSectionName(int32 Section)
{
	return FName(*FString::Printf(TEXT("Attack %d"), Section));
}