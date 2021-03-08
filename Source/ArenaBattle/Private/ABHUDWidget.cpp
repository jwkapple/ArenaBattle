// Fill out your copyright notice in the Description page of Project Settings.

#include "ABHUDWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "ABCharacterStatComponent.h"
#include "ABPlayerState.h"

void UABHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	HPBar = Cast<UProgressBar>(GetWidgetFromName(TEXT("pbHP")));
	EXPBar = Cast<UProgressBar>(GetWidgetFromName(TEXT("pbExp")));
	PlayerName = Cast<UTextBlock>(GetWidgetFromName(TEXT("txtPlayerName")));
	PlayerLevel = Cast<UTextBlock>(GetWidgetFromName(TEXT("txtLevel")));
	CurrentScore = Cast<UTextBlock>(GetWidgetFromName(TEXT("txtCurrentScore")));
	HighScore = Cast<UTextBlock>(GetWidgetFromName(TEXT("txtHighScore")));
}
void UABHUDWidget::BindCharacterStat(UABCharacterStatComponent* NewStat)
{
	CharacterStat = NewStat;
	if (CharacterStat != nullptr)
	{
		CharacterStat->OnHPChange.AddUObject(this, &UABHUDWidget::UpdateCharacterStat);
	}
}
void UABHUDWidget::BindPlayerState(AABPlayerState* NewState)
{
	PlayerState = NewState;
	if (PlayerState != nullptr)
	{
		NewState->OnPlayerStateChanged.AddUObject(this, &UABHUDWidget::UpdatePlayerState);
	}
}

void UABHUDWidget::UpdateCharacterStat()
{
	HPB['ar->SetPercent(CharacterStat->GetHPRatio());
}
void UABHUDWidget::UpdatePlayerState()
{
	PlayerLevel->SetText(FText::FromString(FString::FromInt(PlayerState->GetCharacterLevel())));
	CurrentScore->SetText(FText::FromString(FString::FromInt(PlayerState->GetGameScore())));
}