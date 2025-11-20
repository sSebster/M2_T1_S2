// Fill out your copyright notice in the Description page of Project Settings.


#include "CCapturedPawn.h"

#include "CPoseApplier.h"
#include "CPoseReceiverComponent.h"
#include "Components/PoseableMeshComponent.h"


ACCapturedPawn::ACCapturedPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	PoseableMesh = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("Poseable Mesh"));
	PoseableMesh->SetupAttachment(RootComponent);
	
	PoseReceiver = CreateDefaultSubobject<UCPoseReceiverComponent>(TEXT("Pose Receiver"));
	PoseApplier = CreateDefaultSubobject<UCPoseApplierComponent>(TEXT("Pose Applier"));
}

void ACCapturedPawn::BeginPlay()
{
	Super::BeginPlay();
	
	PoseableMesh->GetBoneNames(BoneNames);
	PoseApplier->Init(PoseableMesh.Get(), PoseReceiver.Get(), CastChecked<APlayerController>(GetController()));
}

void ACCapturedPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ACCapturedPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

