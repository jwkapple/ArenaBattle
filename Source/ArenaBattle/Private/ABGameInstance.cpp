// Fill out your copyright notice in the Description page of Project Settings.


#include "ABGameInstance.h"

UABGameInstance::UABGameInstance()
{
	FString CharacterDataPath = TEXT("/Game/GameData/ABCharacterData.ABCharacterData");

	static ConstructorHelpers::FObjectFinder<UDataTable> DT_ABCHARACTER(*CharacterDataPath);
	if(DT_ABCHARACTER.Succeeded())
	{
		ABCharacterTable = DT_ABCHARACTER.Object;
	}
}

void UABGameInstance::Init()
{
	Super::Init();

	ABLOG(Warning, TEXT("DropExp of Level 20 ABCharacter: %d"), GetABCharacterData(20)->DropExp);
}

FABCharacterData* UABGameInstance::GetABCharacterData(int32 level)
{
	return ABCharacterTable->FindRow<FABCharacterData>(*FString::FromInt(level), TEXT(""));
}
