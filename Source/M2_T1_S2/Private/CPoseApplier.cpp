// Fill out your copyright notice in the Description page of Project Settings.


#include "CPoseApplier.h"

#include "CPoseReceiverComponent.h"
#include "Components/PoseableMeshComponent.h"


UCPoseApplierComponent::UCPoseApplierComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

}

void UCPoseApplierComponent::Init(UPoseableMeshComponent* PoseableMesh, UCPoseReceiverComponent* Receiver)
{
	PoseableMeshComponent = PoseableMesh;
	PoseReceiver = Receiver;
	
	PoseableMeshComponent->GetBoneNames(BoneNames);
}


void UCPoseApplierComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	UE_LOG(LogTemp, Log, TEXT("PoseApplier: TickComponent called"));
	if (!PoseReceiver || !PoseableMeshComponent)
		return;
	
	const TArray<FCPoseLandmark>& Landmarks = PoseReceiver->LastLandmarks;
	if (Landmarks.Num() != 33)
		return;
	
	// Pelvis midpoint (root)
	FCPoseLandmark PelvisLm;
	PelvisLm.X = (Landmarks[23].X + Landmarks[24].X) * 0.5f;
	PelvisLm.Y = (Landmarks[23].Y + Landmarks[24].Y) * 0.5f;
	PelvisLm.Z = (Landmarks[23].Z + Landmarks[24].Z) * 0.5f;
	
	// Shoulder width in normalized units
	float ShoulderWidth = FVector2D(Landmarks[11].X - Landmarks[12].X, Landmarks[11].Y - Landmarks[12].Y).Size();
	float ScaleCm = DesiredShoulderWidthCM / FMath::Max(ShoulderWidth, 0.01f);
	ScaleCm = FMath::Lerp(PrevScaleCM, ScaleCm, 0.2f);
	PrevScaleCM = ScaleCm;
	
	for (const TPair<int32, FName>& It: LandmarkToBone)
	{
		const int32 Index = It.Key;
		const FName BoneName = It.Value;
		
		if (!Landmarks.IsValidIndex(Index) || !BoneNames.Contains(BoneName))
			continue;
		
		const FVector MeshSpacePos = ConvertOne(Landmarks[Index], PelvisLm, ScaleCm, bMirrorY);
		
		PoseableMeshComponent->SetBoneLocationByName(BoneName, MeshSpacePos, EBoneSpaces::ComponentSpace);

		// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("%s: %s"), *BoneName.ToString(), *MeshSpacePos.ToString()));
		DrawDebugPoint(
			GetWorld(),
			PoseableMeshComponent->GetComponentTransform().TransformPosition(MeshSpacePos),
			6.f,
			FColor::Cyan,
			false,
			1.0f
		);
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

FVector UCPoseApplierComponent::ConvertLmToComponent(const TArray<FCPoseLandmark>& Landmarks, float ScaleCM, const FCPoseLandmark& RootPelvis, bool bInMirrorY)
{
	float x = Landmarks[0].X;
	const float xn = (x - RootPelvis.X);
	const float yn = ((1.f - Landmarks[0].Y) - (1.f - RootPelvis.Y));
	const float zn = (-(Landmarks[0].Z - RootPelvis.Z));
	
	float Y = xn * (bInMirrorY ? -1.f : 1.f) * ScaleCM;
	float Z = yn * ScaleCM;
	float X = zn * ScaleCM;
	return FVector(X, Y, Z);
}

