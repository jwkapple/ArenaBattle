// Fill out your copyright notice in the Description page of Project Settings.

#include "ABAIController.h"
#include "BTService_Detect.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTService_Detect::UBTService_Detect()
{
	NodeName = TEXT("Detect");
	Interval = 1.0f;
}

void UBTService_Detect::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	auto ControllingPawn = OwnerComp.GetAIOwner()->GetPawn();
	if (ControllingPawn == nullptr) return;

	auto World = ControllingPawn->GetWorld();
	FVector Center = OwnerComp.GetBlackboardComponent()->GetValueAsVector(AABAIController::HomePosKey);
	float DetectRadius = 600.0f;

	if (World == nullptr) return;
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams CollisionQueryParam(NAME_None, false, ControllingPawn);
}
