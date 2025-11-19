// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CPoseApplier.generated.h"


struct FCPoseLandmark;
class UCPoseReceiverComponent;
class UPoseableMeshComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class M2_T1_S2_API UCPoseApplierComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY()
	UPoseableMeshComponent* PoseableMeshComponent = nullptr;
	UPROPERTY()
	UCPoseReceiverComponent* PoseReceiver = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category="Capture")
	TMap<int32, FName> LandmarkToBone;
	
	UPROPERTY(EditDefaultsOnly, Category="Capture")
	float ScaleMetersToCM = 100.f;
	
	UPROPERTY(EditDefaultsOnly, Category="Capture")
	FVector RootOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category="Capture")
	float DesiredShoulderWidthCM = 30.f;

	UPROPERTY(EditDefaultsOnly, Category="Capture")
	bool bMirrorY = false;
	
	UPROPERTY(EditDefaultsOnly, Category="Capture")
	TArray<FName> BoneNamesToMirror;

	float PrevScaleCM = 30.f;
	
private:
	TArray<FName> BoneNames;
	
public:
	UCPoseApplierComponent();
	
	void Init(UPoseableMeshComponent* PoseableMesh, UCPoseReceiverComponent* Receiver);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
private:
	FVector ConvertOne(const FCPoseLandmark& L, const FCPoseLandmark& Pelvis, float ScaleCM, bool bInMirrorY) const;
	FVector ConvertLmToComponent(const TArray<FCPoseLandmark>& Landmarks, float ScaleCM, const FCPoseLandmark& RootPelvis, bool bInMirrorY);
};
