// Fill out your copyright notice in the Description page of Project Settings.


#include "CPoseApplier.h"

#include "CPoseReceiverComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"


UCPoseApplierComponent::UCPoseApplierComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UCPoseApplierComponent::BeginPlay()
{
    Super::BeginPlay();
    // Pre-create spline chains so they are visible even before first data update (optional)
    if (bEnableSplines)
    {
        EnsureSplineChainCreated(TEXT("Spline_11_13_15"), Spline_11_13_15, Spline_11_13_15_Seg0, Spline_11_13_15_Seg1);
        EnsureSplineChainCreated(TEXT("Spline_12_14_16"), Spline_12_14_16, Spline_12_14_16_Seg0, Spline_12_14_16_Seg1);
        EnsureSplineChainCreated(TEXT("Spline_23_25_27"), Spline_23_25_27, Spline_23_25_27_Seg0, Spline_23_25_27_Seg1);
        EnsureSplineChainCreated(TEXT("Spline_24_26_28"), Spline_24_26_28, Spline_24_26_28_Seg0, Spline_24_26_28_Seg1);
    }
}

void UCPoseApplierComponent::Init(UPoseableMeshComponent* PoseableMesh, UCPoseReceiverComponent* Receiver, APlayerController* InPlayerController)
{
    PoseableMeshComponent = PoseableMesh;
    PoseReceiver = Receiver;
    PlayerController = InPlayerController;
	
	PoseableMeshComponent->GetBoneNames(BoneNames);
}


void UCPoseApplierComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PoseReceiver || !PoseableMeshComponent)
		return;

	// Parse incoming CSV from Python
	TArray<FString> Points;
	PoseReceiver->LastData.ParseIntoArray(Points, TEXT(","), /*CullEmpty*/ false);

	if (Points.Num() != 33 * 3)
	{
		UE_LOG(LogTemp, Verbose, TEXT("PoseApplier: Expected 99 floats, got %d"), Points.Num());
		return;
	}

	constexpr float SmoothingSpeed = 10.f;
	const float Alpha = 1.f - FMath::Exp(-SmoothingSpeed * DeltaTime);
	
	int32 ViewX, ViewY;
	PlayerController->GetViewportSize(ViewX, ViewY);
		
	for (int32 i = 0; i < 33; ++i)
	{
		const int32 BaseIndex = i * 3;
		const float X = FCString::Atof(*Points[BaseIndex]);
		const float Y = FCString::Atof(*Points[BaseIndex + 1]);
		const float Z = FCString::Atof(*Points[BaseIndex + 2]);

		const FVector2D ScreenSpacePos(X * ViewX, Y * ViewY);
		
		FVector WorldOrigin, WorldDirection;
		UGameplayStatics::DeprojectScreenToWorld(PlayerController, ScreenSpacePos, WorldOrigin, WorldDirection);
		
		const float DepthCm = BaseDepthCM + DepthScaleCM * Z;
		FVector WorldPos = WorldOrigin + WorldDirection * DepthCm;
		
		if (FakeBones.IsValidIndex(0))
		{
			const FVector RootPos = FakeBones[0]->GetComponentLocation();
			FVector OffsetFromRoot = WorldPos - RootPos;
			WorldPos = RootPos + OffsetFromRoot * SpreadScale;
		}
		
		FVector CurrentPos = FakeBones[i]->GetComponentLocation();
		// FVector LerpedPos = FMath::VInterpTo(CurrentPos, WorldPos, DeltaTime, Alpha);
		FVector LerpedPos = FMath::Lerp(CurrentPos, WorldPos, Alpha);
		
		FakeBones[i]->SetWorldLocation(LerpedPos);
		// UE_LOG(LogTemp, Log, TEXT("[%i] %s | %s"), i, *ScreenSpacePos.ToString(), *LerpedPos.ToString());
		// DrawDebugPoint(GetWorld(), LerpedPos, 6.f, FColor::Cyan, false, 0.5f);
		
		if (!BoneNamesToMirror.IsValidIndex(i))
			continue;
		
		const FName Name = BoneNamesToMirror[i];
		if (Name == NAME_None)
			continue;
		
		PoseableMeshComponent->SetBoneLocationByName(Name, LerpedPos, EBoneSpaces::WorldSpace);
	}
	
	// Moving pelvis
	const FVector LeftHipPos = FakeBones[23]->GetComponentLocation();
	const FVector RightHipPos = FakeBones[24]->GetComponentLocation();
	const FVector PelvisPos = (LeftHipPos + RightHipPos) * 0.5f;
	PoseableMeshComponent->SetBoneLocationByName(TEXT("pelvis"), PelvisPos, EBoneSpaces::WorldSpace);
	
    // Update visual splines for selected chains
    if (bEnableSplines && SplineStaticMesh && FakeBones.Num() >= 29)
    {
        EnsureSplineChainCreated(TEXT("Spline_11_13_15"), Spline_11_13_15, Spline_11_13_15_Seg0, Spline_11_13_15_Seg1);
        UpdateSplineChain(Spline_11_13_15, Spline_11_13_15_Seg0, Spline_11_13_15_Seg1, 11, 13, 15);

        EnsureSplineChainCreated(TEXT("Spline_12_14_16"), Spline_12_14_16, Spline_12_14_16_Seg0, Spline_12_14_16_Seg1);
        UpdateSplineChain(Spline_12_14_16, Spline_12_14_16_Seg0, Spline_12_14_16_Seg1, 12, 14, 16);

        EnsureSplineChainCreated(TEXT("Spline_23_25_27"), Spline_23_25_27, Spline_23_25_27_Seg0, Spline_23_25_27_Seg1);
        UpdateSplineChain(Spline_23_25_27, Spline_23_25_27_Seg0, Spline_23_25_27_Seg1, 23, 25, 27);

        EnsureSplineChainCreated(TEXT("Spline_24_26_28"), Spline_24_26_28, Spline_24_26_28_Seg0, Spline_24_26_28_Seg1);
        UpdateSplineChain(Spline_24_26_28, Spline_24_26_28_Seg0, Spline_24_26_28_Seg1, 24, 26, 28);
    }
}

FVector UCPoseApplierComponent::ConvertOne(const FCPoseLandmark& L, const FCPoseLandmark& Pelvis, float ScaleCM, bool bInMirrorY) const
{
	const float Xn = (L.X - Pelvis.X);
	const float Yn = ((1.f - L.Y) - (1.f - Pelvis.Y));
	const float Zn = (-(L.Z - Pelvis.Z));

	const float Y = Xn * (bMirrorY ? -1.f : 1.f) * ScaleCM;
	const float Z = Yn * ScaleCM;
	const float X = Zn * ScaleCM;

	return FVector(X, Y, Z);
}

void UCPoseApplierComponent::EnsureSplineChainCreated(const FName& BaseName,
    TObjectPtr<USplineComponent>& OutSpline,
    TObjectPtr<USplineMeshComponent>& OutSeg0,
    TObjectPtr<USplineMeshComponent>& OutSeg1)
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
        return;

    USceneComponent* Root = OwnerActor->GetRootComponent();

    if (!OutSpline)
    {
        OutSpline = NewObject<USplineComponent>(OwnerActor, USplineComponent::StaticClass(), BaseName);
        OutSpline->SetMobility(EComponentMobility::Movable);
        OutSpline->RegisterComponent();
        if (Root)
        {
            OutSpline->AttachToComponent(Root, FAttachmentTransformRules::KeepWorldTransform);
        }
        // Ensure it has 3 points
        OutSpline->ClearSplinePoints(false);
        OutSpline->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::Local, false);
        OutSpline->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::Local, false);
        OutSpline->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::Local, false);
        OutSpline->SetClosedLoop(false, false);
        OutSpline->UpdateSpline();
    }

    auto CreateSeg = [&](TObjectPtr<USplineMeshComponent>& Seg, const FString& Suffix)
    {
        if (!Seg)
        {
            const FName CompName(*(BaseName.ToString() + TEXT("_") + Suffix));
            Seg = NewObject<USplineMeshComponent>(OwnerActor, USplineMeshComponent::StaticClass(), CompName);
            Seg->SetMobility(EComponentMobility::Movable);
            Seg->SetStaticMesh(SplineStaticMesh);
            if (SplineMaterial)
            {
                Seg->SetMaterial(0, SplineMaterial);
            }
            Seg->SetForwardAxis(ESplineMeshAxis::X, false);
            Seg->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Seg->RegisterComponent();
            // Attach to the spline so we can use its local space
            Seg->AttachToComponent(OutSpline, FAttachmentTransformRules::KeepRelativeTransform);
        }
    };

    CreateSeg(OutSeg0, TEXT("Seg0"));
    CreateSeg(OutSeg1, TEXT("Seg1"));
}

void UCPoseApplierComponent::UpdateSplineChain(TObjectPtr<USplineComponent>& InSpline,
    TObjectPtr<USplineMeshComponent>& InSeg0,
    TObjectPtr<USplineMeshComponent>& InSeg1,
    int32 Idx0, int32 Idx1, int32 Idx2)
{
    if (!InSpline || !InSeg0 || !InSeg1 || !SplineStaticMesh)
        return;
    if (!FakeBones.IsValidIndex(Idx0) || !FakeBones.IsValidIndex(Idx1) || !FakeBones.IsValidIndex(Idx2))
        return;
    USceneComponent* B0 = FakeBones[Idx0];
    USceneComponent* B1 = FakeBones[Idx1];
    USceneComponent* B2 = FakeBones[Idx2];
    if (!B0 || !B1 || !B2)
        return;

    const FVector P0 = B0->GetComponentLocation();
    const FVector P1 = B1->GetComponentLocation();
    const FVector P2 = B2->GetComponentLocation();

    // Update spline points in world space
    InSpline->SetLocationAtSplinePoint(0, P0, ESplineCoordinateSpace::World, false);
    InSpline->SetLocationAtSplinePoint(1, P1, ESplineCoordinateSpace::World, false);
    InSpline->SetLocationAtSplinePoint(2, P2, ESplineCoordinateSpace::World, false);
    InSpline->UpdateSpline();

    const FTransform SplineXform = InSpline->GetComponentTransform();
    const FVector L0 = SplineXform.InverseTransformPosition(P0);
    const FVector L1 = SplineXform.InverseTransformPosition(P1);
    const FVector L2 = SplineXform.InverseTransformPosition(P2);

    auto SetSeg = [&](USplineMeshComponent* Seg, const FVector& A, const FVector& B)
    {
        const FVector AB = (B - A);
        const float Len = AB.Length();
        const FVector Tangent = (Len > KINDA_SMALL_NUMBER) ? (AB * 0.5f) : FVector::ZeroVector;
        // Ensure the mesh updates immediately when endpoints change
        Seg->SetStartAndEnd(A, Tangent, B, Tangent, true);
        Seg->SetStartScale(FVector2D(SplineStartScale));
        Seg->SetEndScale(FVector2D(SplineEndScale));
        Seg->UpdateMesh();
    };

    SetSeg(InSeg0, L0, L1);
    SetSeg(InSeg1, L1, L2);
}


