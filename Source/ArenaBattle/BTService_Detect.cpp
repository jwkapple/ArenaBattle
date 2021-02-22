// Fill out your copyright notice in the Description page of Project Settings.

#include "ABAIController.h"
#include "ABCharacter.h"
#include "DrawDebugHelpers.h"
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
	bool bResult = World->OverlapMultiByChannel(
		OverlapResults,
		Center,
		FQuat::Identity,
		ECollisionChannel::ECC_GameTraceChannel2,
		FCollisionShape::MakeSphere(DetectRadius),
		CollisionQueryParam
	);

	if(bResult)
	{
		OwnerComp.GetBlackboardComponent()->SetValueAsObject(AABAIController::TargetKey, nullptr);
		for(auto const& OverlapResult : OverlapResults)
		{
			AABCharacter* ABCharacter = Cast<AABCharacter>(OverlapResult.GetActor());
			if(ABCharacter && ABCharacter->GetController()->IsPlayerController())
			{
				OwnerComp.GetBlackboardComponent()->SetValueAsObject(AABAIController::TargetKey, ABCharacter);
				DrawDebugSphere(World, Center, DetectRadius, 16, FColor::Red, false, 0.5f);
				DrawDebugPoint(World, ABCharacter->GetActorLocation(), 10.0f, FColor::Blue, false, 0.2f);
				return;
			}

			DrawDebugLine(World, Center, ABCharacter->GetActorLocation(), FColor::Green, false, 0.5f);
		}
	}
}
