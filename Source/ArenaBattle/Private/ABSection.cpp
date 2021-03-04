// Fill out your copyright notice in the Description page of Project Settings.


#include "ABSection.h"
#include "ABCharacter.h"
#include "ABItemBox.h"


// Sets default values
AABSection::AABSection()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	bNoBattle = false;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MESH"));
	RootComponent = Mesh;

	FString AssetPath = TEXT("/Game/Book/StaticMesh/SM_SQUARE.SM_SQUARE");

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SM_SQUARE(*AssetPath);
	if (SM_SQUARE.Succeeded())
	{
		Mesh->SetStaticMesh(SM_SQUARE.Object);
		ABLOG(Warning, TEXT("Succeded Loading SM_SQUARE Asset"));
	}

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("TRIGGER"));
	Trigger->SetBoxExtent(FVector(775.0f, 775.0f, 300.0f));
	Trigger->SetupAttachment(RootComponent);
	Trigger->SetRelativeLocation(FVector(0.0f, 0.0f, 250.0f));
	Trigger->SetCollisionProfileName(TEXT("ABTrigger"));

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AABSection::OnTriggerBeginOverlap);

	FString GateAssetPath = TEXT("/Game/Book/StaticMesh/SM_GATE.SM_GATE");
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SM_GATE(*GateAssetPath);
	if (!SM_GATE.Succeeded()) ABLOG_S(Warning);
	
	static FName GateSockets[] = { {TEXT("+XGate")}, {TEXT("-XGate")},{TEXT("+YGate")},{TEXT("-YGate")} };
	for (FName GateSocket : GateSockets)
	{
		UStaticMeshComponent* NewGate = CreateDefaultSubobject<UStaticMeshComponent>(*GateSocket.ToString());
		NewGate->SetStaticMesh(SM_GATE.Object);
		NewGate->SetupAttachment(RootComponent, GateSocket);
		NewGate->SetRelativeLocation(FVector(0.0f, -80.5f, 0.0f));
		GateMeshes.Add(NewGate);

		UBoxComponent* GateTrigger = CreateDefaultSubobject<UBoxComponent>(*GateSocket.ToString().Append(TEXT("Trigger")));
		GateTrigger->SetupAttachment(RootComponent, GateSocket);
		GateTrigger->SetBoxExtent(FVector(100.0f, 100.0f, 300.0f));
		GateTrigger->SetRelativeLocation(FVector(70.0f, 0.0f, 250.0f));
		GateTrigger->SetCollisionProfileName(TEXT("ABTrigger"));
		GateTriggers.Add(GateTrigger);

		GateTrigger->OnComponentBeginOverlap.AddDynamic(this, &AABSection::OnGateTriggerBeginOverlap);
		GateTrigger->ComponentTags.Add(GateSocket);
	}


	EnemySpawnTime = 2.0f;
	ItemSpawnTime = 5.0f;
}

// Called when the game starts or when spawned
void AABSection::BeginPlay()
{
	Super::BeginPlay();
	
	SetState(bNoBattle ? ESectionState::COMPLETE : ESectionState::READY);
}

// Called every frame
void AABSection::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AABSection::SetState(ESectionState NewState)
{
	switch (NewState)
	{
	case AABSection::ESectionState::READY:
	{
		ABLOG(Warning, TEXT("State:: READY"));
		Trigger->SetCollisionProfileName(TEXT("ABTrigger"));

		
		for (UBoxComponent* GateTrigger : GateTriggers)
		{
			GateTrigger->SetCollisionProfileName(TEXT("NoCollision"));
		}

		ABLOG(Warning, TEXT("Base Trigger: %s"), *Trigger->GetCollisionProfileName().ToString());
		ABLOG(Warning, TEXT("Gate Trigger: %s"), *GateTriggers[0]->GetCollisionProfileName().ToString());
		OperateGates(true);

		break;
	}
		
	case AABSection::ESectionState::BATTLE:
	{
		ABLOG(Warning, TEXT("State:: BATTLE"));
		Trigger->SetCollisionProfileName(TEXT("NoCollision"));

		for (UBoxComponent* GateTrigger : GateTriggers)
		{
			GateTrigger->SetCollisionProfileName(TEXT("NoCollision"));
		}

		ABLOG(Warning, TEXT("Base Trigger: %s"), *Trigger->GetCollisionProfileName().ToString());
		ABLOG(Warning, TEXT("Gate Trigger: %s"), *GateTriggers[0]->GetCollisionProfileName().ToString());
		OperateGates(false);

		GetWorld()->GetTimerManager().SetTimer(SpawnNPCTimerHandle, FTimerDelegate::CreateUObject(this, &AABSection::OnNPCSpawn), EnemySpawnTime, false);
		GetWorld()->GetTimerManager().SetTimer(SpawnItemTimerHandle, FTimerDelegate::CreateLambda([this]()-> void {

			FVector2D RandPos = FMath::RandPointInCircle(600.0f);
			GetWorld()->SpawnActor<AABItemBox>(GetActorLocation() + FVector(RandPos, 30.0f), FRotator::ZeroRotator);
		}), ItemSpawnTime, false);
		break;
	}
	case AABSection::ESectionState::COMPLETE:
	{
		ABLOG(Warning, TEXT("State:: COMPLETE"));
		Trigger->SetCollisionProfileName(TEXT("NoCollision"));

		for (UBoxComponent* GateTrigger : GateTriggers)
		{
			GateTrigger->SetCollisionProfileName(TEXT("ABTrigger"));
		}

		ABLOG(Warning, TEXT("Base Trigger: %s"), *Trigger->GetCollisionProfileName().ToString());
		ABLOG(Warning, TEXT("Gate Trigger: %s"), *GateTriggers[0]->GetCollisionProfileName().ToString());
		OperateGates(true);

		break;
	}
	}


	CurrentState = NewState;
}

void AABSection::OperateGates(bool bOpen) 
{
	for (auto Gate : GateMeshes)
	{
		Gate->SetRelativeRotation(bOpen ? FRotator(0.0f, -90.0f, 0.0f) : FRotator::ZeroRotator);
	}
}

void AABSection::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool
	bFromSweep, const FHitResult &SweepResult)
{
	if (CurrentState == ESectionState::READY)
	{
		SetState(ESectionState::BATTLE);
	}
}

void AABSection::OnGateTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool
	bFromSweep, const FHitResult &SweepResult)
{
	FName ComponentTag = OverlappedComponent->ComponentTags[0];
	FName SocketName = *ComponentTag.ToString().Left(2);
	if (!Mesh->DoesSocketExist(SocketName)) return;

	FVector NewLocation = Mesh->GetSocketLocation(SocketName);

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams CollsionQueryParam(NAME_None, false, this);
	FCollisionObjectQueryParams ObjectQueryParam(FCollisionObjectQueryParams::AllObjects);
	bool bResult = GetWorld()->OverlapMultiByObjectType(OverlapResults, NewLocation, FQuat::Identity, ObjectQueryParam,
		FCollisionShape::MakeSphere(750.0f), CollsionQueryParam);
	if (!bResult)
	{
		auto NewSection = GetWorld()->SpawnActor<AABSection>(NewLocation, FRotator::ZeroRotator);
	}
	else
	{
		ABLOG(Warning, TEXT("Failed To Spawn New AABSection No: %s"), *SocketName.ToString());
	}
}

void AABSection::OnNPCSpawn()
{
	FVector2D RandXY = FMath::RandPointInCircle(600.0f);
	GetWorld()->SpawnActor<AABCharacter>(GetActorLocation() + FVector::UpVector * 88.0f, FRotator::ZeroRotator);
}

