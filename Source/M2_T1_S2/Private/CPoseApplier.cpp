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
    // Pre-create dynamic spline chains so they are visible even before first data update (optional)
    if (bEnableSplines)
    {
        EnsureDynamicChainsCreated();
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
	
    // Update visual splines for configured chains
    if (bEnableSplines && SplineStaticMesh)
    {
        EnsureDynamicChainsCreated();
        UpdateDynamicChains();
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

void UCPoseApplierComponent::EnsureDynamicChainsCreated()
{
    if (!bEnableSplines || !SplineStaticMesh)
	    return;

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
	    return;

    USceneComponent* Root = OwnerActor->GetRootComponent();

    // Resize runtime array to match configs
    if (RuntimeChains.Num() != SplineChains.Num())
    {
        // Best effort cleanup of previous components if any
        for (FSplineRuntimeChain& Chain : RuntimeChains)
        {
            if (Chain.Spline)
            {
                Chain.Spline->DestroyComponent();
            }
            for (USplineMeshComponent* Seg : Chain.Segments)
            {
                if (Seg)
                {
                    Seg->DestroyComponent();
                }
            }
        }
        RuntimeChains.Empty(SplineChains.Num());
        RuntimeChains.SetNum(SplineChains.Num());
    }

    for (int32 i = 0; i < SplineChains.Num(); ++i)
    {
        const FSplineChainConfig& Config = SplineChains[i];
        FSplineRuntimeChain& RT = RuntimeChains[i];

        const int32 NumPoints = Config.Points.Num();
        const int32 DesiredSegments = FMath::Max(0, NumPoints - 1);

        if (!RT.Spline)
        {
            const FName SplineName = Config.DebugName.IsNone() ? FName(*FString::Printf(TEXT("DynSpline_%d"), i))
                                                              : FName(*FString::Printf(TEXT("DynSpline_%d_%s"), i, *Config.DebugName.ToString()));
            RT.Spline = NewObject<USplineComponent>(OwnerActor, USplineComponent::StaticClass(), SplineName);
            RT.Spline->SetMobility(EComponentMobility::Movable);
            RT.Spline->RegisterComponent();
            if (Root)
            {
                RT.Spline->AttachToComponent(Root, FAttachmentTransformRules::KeepWorldTransform);
            }
        }

        // Ensure spline has NumPoints points
        if (RT.Spline->GetNumberOfSplinePoints() != NumPoints)
        {
            RT.Spline->ClearSplinePoints(false);
            for (int32 p = 0; p < NumPoints; ++p)
            {
                RT.Spline->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::Local, false);
            }
            RT.Spline->SetClosedLoop(false, false);
            RT.Spline->UpdateSpline();
        }

        // Ensure segments count matches
        if (RT.Segments.Num() != DesiredSegments)
        {
            // Destroy old
            for (USplineMeshComponent* Seg : RT.Segments)
            {
                if (Seg)
                {
                    Seg->DestroyComponent();
                }
            }
            RT.Segments.Empty(DesiredSegments);
            RT.Segments.Reserve(DesiredSegments);

            for (int32 s = 0; s < DesiredSegments; ++s)
            {
                const FName SegName(*FString::Printf(TEXT("%s_Seg_%d"), *RT.Spline->GetName(), s));
                USplineMeshComponent* Seg = NewObject<USplineMeshComponent>(OwnerActor, USplineMeshComponent::StaticClass(), SegName);
                Seg->SetMobility(EComponentMobility::Movable);
                Seg->SetStaticMesh(SplineStaticMesh);

                // Apply materials: either provided SplineMaterial to all slots or copy from mesh
                int32 NumSlots = 0;
                if (SplineStaticMesh)
                {
                    NumSlots = SplineStaticMesh->GetStaticMaterials().Num();
                }
                if (NumSlots <= 0)
                {
                    NumSlots = Seg->GetNumMaterials();
                }
                for (int32 Slot = 0; Slot < NumSlots; ++Slot)
                {
                    if (SplineMaterial)
                    {
                        Seg->SetMaterial(Slot, SplineMaterial);
                    }
                    else if (SplineStaticMesh)
                    {
                        if (UMaterialInterface* MeshMat = SplineStaticMesh->GetMaterial(Slot))
                        {
                            Seg->SetMaterial(Slot, MeshMat);
                        }
                    }
                }

                Seg->SetForwardAxis(ESplineMeshAxis::X, false);
                Seg->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                Seg->RegisterComponent();
                Seg->AttachToComponent(RT.Spline, FAttachmentTransformRules::KeepRelativeTransform);

                RT.Segments.Add(Seg);
            }
        }
    }
}

void UCPoseApplierComponent::UpdateDynamicChains()
{
    if (!bEnableSplines || !SplineStaticMesh)
	    return;

    for (int32 i = 0; i < SplineChains.Num(); ++i)
    {
        const FSplineChainConfig& Config = SplineChains[i];
        FSplineRuntimeChain& RT = RuntimeChains.IsValidIndex(i) ? RuntimeChains[i] : *new FSplineRuntimeChain();
        if (!RT.Spline)
	        continue;

        const int32 NumPoints = Config.Points.Num();
        if (NumPoints < 2)
	        continue;

        // Gather world and local positions and weights
        TArray<FVector> WorldPts;
        WorldPts.SetNum(NumPoints);
        TArray<FVector> LocalPts;
        LocalPts.SetNum(NumPoints);
        TArray<float> Weights;
        Weights.SetNum(NumPoints);

        bool bAllValid = true;
        for (int32 p = 0; p < NumPoints; ++p)
        {
            const int32 LM = Config.Points[p].LandmarkIndex;
            const float W = Config.Points[p].Weight;
            Weights[p] = W;
            if (!FakeBones.IsValidIndex(LM) || !FakeBones[LM])
            {
                bAllValid = false;
                break;
            }
            const FVector P = FakeBones[LM]->GetComponentLocation();
            WorldPts[p] = P;
        }
    	
        if (!bAllValid)
	        continue;

        // Update spline points in world space
        for (int32 p = 0; p < NumPoints; ++p)
	        RT.Spline->SetLocationAtSplinePoint(p, WorldPts[p], ESplineCoordinateSpace::World, false);
    	
        RT.Spline->UpdateSpline();

        const FTransform SplineXform = RT.Spline->GetComponentTransform();
        for (int32 p = 0; p < NumPoints; ++p)
	        LocalPts[p] = SplineXform.InverseTransformPosition(WorldPts[p]);

        // Update segments
        for (int32 s = 0; s < NumPoints - 1; ++s)
        {
            USplineMeshComponent* Seg = RT.Segments.IsValidIndex(s) ? RT.Segments[s] : nullptr;
            if (!Seg)
	            continue;
        	
            const FVector A = LocalPts[s];
            const FVector B = LocalPts[s + 1];
            const FVector AB = B - A;
            const float Len = AB.Length();
            const FVector Tangent = (Len > KINDA_SMALL_NUMBER) ? (AB * 0.5f) : FVector::ZeroVector;

            Seg->SetStartAndEnd(A, Tangent, B, Tangent, true);
            const float StartW = Weights[s] * WidthPerWeight;
            const float EndW = Weights[s + 1] * WidthPerWeight;
            Seg->SetStartScale(FVector2D(StartW, StartW));
            Seg->SetEndScale(FVector2D(EndW, EndW));
            Seg->UpdateMesh();
        }
    }
}


