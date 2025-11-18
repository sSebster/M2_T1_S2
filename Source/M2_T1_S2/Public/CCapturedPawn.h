// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "CCapturedPawn.generated.h"

class UCPoseReceiverComponent;
class UPoseableMeshComponent;

UCLASS(Blueprintable)
class M2_T1_S2_API ACCapturedPawn : public APawn
{
	GENERATED_BODY()

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Capture", meta=(AllowPrivateAccess=true))
	TObjectPtr<UPoseableMeshComponent> PoseableMesh;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Capture", meta=(AllowPrivateAccess=true))
	TObjectPtr<UCPoseReceiverComponent> PoseReceiver;
	
public:
	ACCapturedPawn();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
