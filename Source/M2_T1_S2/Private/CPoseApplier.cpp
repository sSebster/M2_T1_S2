// Fill out your copyright notice in the Description page of Project Settings.


#include "CPoseApplier.h"

#include "CPoseReceiverComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Kismet/GameplayStatics.h"


UCPoseApplierComponent::UCPoseApplierComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
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

	const float SmoothingSpeed = 10.f;
	const float Alpha = 1.f - FMath::Exp(-SmoothingSpeed * DeltaTime);
	
	int32 ViewX, ViewY;
	PlayerController->GetViewportSize(ViewX, ViewY);
		
	for (int32 i = 0; i < 33; ++i)
	{
		// if (!BoneNamesToMirror.IsValidIndex(i))
		// {
		// 	continue;
		// }
		//
		// const FName Name = BoneNamesToMirror[i];
		// if (Name == NAME_None)
		// 	continue;

		int32 BaseIndex = i * 3;
		float X = FCString::Atof(*Points[BaseIndex]);
		float Y = FCString::Atof(*Points[BaseIndex + 1]);
		float Z = FCString::Atof(*Points[BaseIndex + 2]);

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
		DrawDebugPoint(GetWorld(), LerpedPos, 6.f, FColor::Cyan, false, 0.5f);
		
		// PoseableMeshComponent->SetBoneTransformByName(
		// 	Name,
		// 	Transform,
		// 	EBoneSpaces::WorldSpace);
	}
}

FVector UCPoseApplierComponent::ConvertOne(const FCPoseLandmark& L, const FCPoseLandmark& Pelvis, float ScaleCM, bool bInMirrorY) const
{
	const float xn = (L.X - Pelvis.X);
	const float yn = ((1.f - L.Y) - (1.f - Pelvis.Y));
	const float zn = (-(L.Z - Pelvis.Z));

	const float Y = xn * (bMirrorY ? -1.f : 1.f) * ScaleCM;
	const float Z = yn * ScaleCM;
	const float X = zn * ScaleCM;

	return FVector(X, Y, Z);
}


