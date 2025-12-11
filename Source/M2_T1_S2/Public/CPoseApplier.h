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

USTRUCT(BlueprintType)
struct FWeightedLandmark
{
    GENERATED_BODY()

    // Index into FakeBones / incoming landmark array (0..32 for MediaPipe body)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Capture|Spline")
    int32 LandmarkIndex = 0;

    // Thickness weight at this point. Final width = Weight * WidthPerWeight
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Capture|Spline")
    float Weight = 1.0f;
};

USTRUCT(BlueprintType)
struct FSplineChainConfig
{
    GENERATED_BODY()

    // Just a label to quickly identify the chain in the component (optional)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Capture|Spline")
    FName DebugName;

    // List of weighted landmarks forming this chain, in order. Needs at least 2 points.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Capture|Spline")
    TArray<FWeightedLandmark> Points;
};

USTRUCT()
struct FSplineRuntimeChain
{
    GENERATED_BODY()

    UPROPERTY(Transient)
    TObjectPtr<USplineComponent> Spline = nullptr;

    UPROPERTY(Transient)
    TArray<TObjectPtr<USplineMeshComponent>> Segments;
};

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

 // Visual splines are driven by designer-authored chains
 UPROPERTY(EditAnywhere, Category="Capture|Spline")
 bool bEnableSplines = true;

 // Designer-configurable chains of landmarks
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Capture|Spline")
 TArray<FSplineChainConfig> SplineChains;

 // Mesh used by SplineMeshComponents (ideally a cylinder authored along +X)
 UPROPERTY(EditAnywhere, Category="Capture|Spline")
 TObjectPtr<UStaticMesh> SplineStaticMesh = nullptr;

 // Optional override: if set, applied to all material slots of each segment
 UPROPERTY(EditAnywhere, Category="Capture|Spline")
 TObjectPtr<UMaterialInterface> SplineMaterial = nullptr;

 // Width per unit weight. Final width at a point = Weight * WidthPerWeight
 UPROPERTY(EditAnywhere, Category="Capture|Spline", meta=(ClampMin="0.0"))
 float WidthPerWeight = 0.2f;

private:
	TArray<FName> BoneNames;
	
	UPROPERTY()
	APlayerController* PlayerController = nullptr;
	UPROPERTY()
	UPoseableMeshComponent* PoseableMeshComponent = nullptr;
 UPROPERTY()
 UCPoseReceiverComponent* PoseReceiver = nullptr;

 // Runtime data for spawned spline components per configured chain
 UPROPERTY(Transient)
 TArray<FSplineRuntimeChain> RuntimeChains;
	
public:
    UCPoseApplierComponent();

    virtual void BeginPlay() override;
	
	void Init(UPoseableMeshComponent* PoseableMesh, UCPoseReceiverComponent* Receiver, APlayerController* InPlayerController);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
private:
    FVector ConvertOne(const FCPoseLandmark& L, const FCPoseLandmark& Pelvis, float ScaleCM, bool bInMirrorY) const;

    void EnsureDynamicChainsCreated();
    void UpdateDynamicChains();
};
