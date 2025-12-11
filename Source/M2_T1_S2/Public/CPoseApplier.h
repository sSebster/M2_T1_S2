// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CPoseApplier.generated.h"


struct FCPoseLandmark;
class UCPoseReceiverComponent;
class UPoseableMeshComponent;
class USplineComponent;
class USplineMeshComponent;
class UStaticMesh;
class UMaterialInterface;

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

	// Visual splines between selected landmark chains (e.g., 11-13-15, ...)
	UPROPERTY(EditDefaultsOnly, Category="Capture|Spline")
	bool bEnableSplines = true;

	// Mesh used by SplineMeshComponents (ideally a cylinder authored along +X)
	UPROPERTY(EditDefaultsOnly, Category="Capture|Spline")
	TObjectPtr<UStaticMesh> SplineStaticMesh = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Capture|Spline")
	TObjectPtr<UMaterialInterface> SplineMaterial = nullptr;

	// Y/Z scale for the spline mesh (relative to the mesh import size)
	UPROPERTY(EditDefaultsOnly, Category="Capture|Spline")
	FVector2D SplineStartScale = FVector2D(0.2f, 0.2f);

	UPROPERTY(EditDefaultsOnly, Category="Capture|Spline")
	FVector2D SplineEndScale = FVector2D(0.2f, 0.2f);

private:
	TArray<FName> BoneNames;
	
	UPROPERTY()
	APlayerController* PlayerController = nullptr;
	UPROPERTY()
	UPoseableMeshComponent* PoseableMeshComponent = nullptr;
	UPROPERTY()
	UCPoseReceiverComponent* PoseReceiver = nullptr;
    
	// One spline per chain (3 points -> 2 segments)
	UPROPERTY(Transient)
	TObjectPtr<USplineComponent> Spline_11_13_15 = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<USplineMeshComponent> Spline_11_13_15_Seg0 = nullptr; // 11->13
	UPROPERTY(Transient)
	TObjectPtr<USplineMeshComponent> Spline_11_13_15_Seg1 = nullptr; // 13->15

	UPROPERTY(Transient)
	TObjectPtr<USplineComponent> Spline_12_14_16 = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<USplineMeshComponent> Spline_12_14_16_Seg0 = nullptr; // 12->14
	UPROPERTY(Transient)
	TObjectPtr<USplineMeshComponent> Spline_12_14_16_Seg1 = nullptr; // 14->16

	UPROPERTY(Transient)
	TObjectPtr<USplineComponent> Spline_23_25_27 = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<USplineMeshComponent> Spline_23_25_27_Seg0 = nullptr; // 23->25
	UPROPERTY(Transient)
	TObjectPtr<USplineMeshComponent> Spline_23_25_27_Seg1 = nullptr; // 25->27

	UPROPERTY(Transient)
	TObjectPtr<USplineComponent> Spline_24_26_28 = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<USplineMeshComponent> Spline_24_26_28_Seg0 = nullptr; // 24->26
	UPROPERTY(Transient)
	TObjectPtr<USplineMeshComponent> Spline_24_26_28_Seg1 = nullptr; // 26->28
	
public:
    UCPoseApplierComponent();

    virtual void BeginPlay() override;
	
	void Init(UPoseableMeshComponent* PoseableMesh, UCPoseReceiverComponent* Receiver, APlayerController* InPlayerController);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
private:
    FVector ConvertOne(const FCPoseLandmark& L, const FCPoseLandmark& Pelvis, float ScaleCM, bool bInMirrorY) const;

    void EnsureSplineChainCreated(const FName& BaseName,
        TObjectPtr<USplineComponent>& OutSpline,
        TObjectPtr<USplineMeshComponent>& OutSeg0,
        TObjectPtr<USplineMeshComponent>& OutSeg1);

    void UpdateSplineChain(TObjectPtr<USplineComponent>& InSpline,
        TObjectPtr<USplineMeshComponent>& InSeg0,
        TObjectPtr<USplineMeshComponent>& InSeg1,
        int32 Idx0, int32 Idx1, int32 Idx2);
};
