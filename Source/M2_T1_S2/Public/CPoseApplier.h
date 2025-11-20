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

	// How far in front of the camera to place the landmarks (in cm)
	UPROPERTY(EditDefaultsOnly, Category="Capture|Fake")
	float BaseDepthCM = 150.f;

	// How much the incoming Z value affects depth (in cm)
	UPROPERTY(EditDefaultsOnly, Category="Capture|Fake")
	float DepthScaleCM = 300.f;

	// Multiplier to increase the distance between landmarks in world space
	UPROPERTY(EditDefaultsOnly, Category="Capture|Fake")
	float SpreadScale = 1.5f;
	
	UPROPERTY(BlueprintReadWrite, Category="Capture|Fake", meta=(AllowPrivateAccess=true))
	TArray<TObjectPtr<USceneComponent>> FakeBones;
	
private:
	TArray<FName> BoneNames;
	
	UPROPERTY()
	APlayerController* PlayerController = nullptr;
	UPROPERTY()
	UPoseableMeshComponent* PoseableMeshComponent = nullptr;
	UPROPERTY()
	UCPoseReceiverComponent* PoseReceiver = nullptr;
	
public:
	UCPoseApplierComponent();
	
	void Init(UPoseableMeshComponent* PoseableMesh, UCPoseReceiverComponent* Receiver, APlayerController* InPlayerController);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
private:
	FVector ConvertOne(const FCPoseLandmark& L, const FCPoseLandmark& Pelvis, float ScaleCM, bool bInMirrorY) const;
};
