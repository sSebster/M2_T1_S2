// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "CCapturedPawn.generated.h"

class UCPoseApplierComponent;
class UCPoseReceiverComponent;
class UPoseableMeshComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class M2_T1_S2_API ACCapturedPawn : public APawn
{
	GENERATED_BODY()

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Capture", meta=(AllowPrivateAccess=true))
	TObjectPtr<UPoseableMeshComponent> PoseableMesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Capture", meta=(AllowPrivateAccess=true))
	TObjectPtr<UCPoseReceiverComponent> PoseReceiver;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Capture", meta=(AllowPrivateAccess=true))
	TObjectPtr<UCPoseApplierComponent> PoseApplier;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Capture", meta=(AllowPrivateAccess=true))
	TMap<int32, FName> LandmarkToBone;

	// 33 helper scene components to visualize/apply incoming pose points
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="Capture", meta=(AllowPrivateAccess=true))
	TArray<TObjectPtr<UStaticMeshComponent>> FakeBoneMeshes;
	
	UPROPERTY()
	TArray<FName> BoneNames;
	
public:
	ACCapturedPawn();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
