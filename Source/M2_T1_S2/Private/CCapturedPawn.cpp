// Fill out your copyright notice in the Description page of Project Settings.


#include "CCapturedPawn.h"

#include "CPoseApplier.h"
#include "CPoseReceiverComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"


ACCapturedPawn::ACCapturedPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    PoseableMesh = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("Poseable Mesh"));
    PoseableMesh->SetupAttachment(RootComponent);
    
    PoseReceiver = CreateDefaultSubobject<UCPoseReceiverComponent>(TEXT("Pose Receiver"));
    PoseApplier = CreateDefaultSubobject<UCPoseApplierComponent>(TEXT("Pose Applier"));

    // Create 33 helper static mesh components to represent pose landmarks
    FakeBoneMeshes.Reserve(33);
    for (int32 i = 0; i < 33; ++i)
    {
        const FString CompName = FString::Printf(TEXT("FakeBone_%02d"), i);
        UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(*CompName);
        Comp->SetupAttachment(RootComponent);
        Comp->SetMobility(EComponentMobility::Movable);
        Comp->SetHiddenInGame(false);
        Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        FakeBoneMeshes.Add(Comp);
    }
}

void ACCapturedPawn::BeginPlay()
{
    Super::BeginPlay();
    
    PoseableMesh->GetBoneNames(BoneNames);
    PoseApplier->Init(PoseableMesh.Get(), PoseReceiver.Get(), CastChecked<APlayerController>(GetController()));

    // Pass the 33 helper components to the pose applier as scene components
    PoseApplier->FakeBones.Empty();
    PoseApplier->FakeBones.Reserve(FakeBoneMeshes.Num());
    for (UStaticMeshComponent* SM : FakeBoneMeshes)
    {
        PoseApplier->FakeBones.Add(SM);
    }
}

void ACCapturedPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ACCapturedPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

