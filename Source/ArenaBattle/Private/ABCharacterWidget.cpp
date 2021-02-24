// Fill out your copyright notice in the Description page of Project Settings.


#include "ABCharacterWidget.h"
#include "ABCharacterStatComponent.h"
#include "Components/ProgressBar.h"

void UABCharacterWidget::BindCharacterStatComponent(UABCharacterStatComponent* NewCharacterStat)
{
	CurrentCharacterStat = NewCharacterStat;

	if(CurrentCharacterStat != nullptr)
	{
		NewCharacterStat->OnHPChange.AddUObject(this, &UABCharacterWidget::UpdateHPWidget);
	}
}

void UABCharacterWidget::NativeConstruct()
{
	Super::NativeConstruct();
	HPProgressBar = Cast<UProgressBar>(GetWidgetFromName(TEXT("PB_HPBar")));
	UpdateHPWidget();
}

void UABCharacterWidget::UpdateHPWidget()
{
	if(CurrentCharacterStat.IsValid())
	{
		if(HPProgressBar != nullptr)
		{
			HPProgressBar->SetPercent(CurrentCharacterStat->GetHPRatio());
		}
	}
}
