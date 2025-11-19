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


void UCPoseApplierComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PoseReceiver || !PoseableMeshComponent)
		return;

	// --- Parse incoming CSV from Python ---
	TArray<FString> Points;
	PoseReceiver->LastData.ParseIntoArray(Points, TEXT(","), /*CullEmpty*/ false);

	if (Points.Num() != 33 * 3)
	{
		// Optional: log once to verify mismatch
		UE_LOG(LogTemp, Verbose,
			   TEXT("PoseApplier: Expected 99 floats, got %d"), Points.Num());
		return;
	}

	const float Scale = 100.f; // meters -> cm (or tweak as needed)

	// --- Interpolation parameters ---
	const float SmoothingSpeed = 10.f; // tweak: higher = snappier, lower = smoother
	const float Alpha = 1.f - FMath::Exp(-SmoothingSpeed * DeltaTime);

	for (int32 i = 0; i < 33; ++i)
	{
		if (!BoneNamesToMirror.IsValidIndex(i))
		{
			// Prevent out-of-bounds access if not enough entries are set in the array
			continue;
		}

		const FName Name = BoneNamesToMirror[i];
		if (Name == NAME_None)
			continue;

		const int32 BaseIndex = i * 3;
		const float X = FCString::Atof(*Points[BaseIndex + 0]) * Scale;
		const float Y = FCString::Atof(*Points[BaseIndex + 1]) * Scale;
		const float Z = FCString::Atof(*Points[BaseIndex + 2]) * Scale;

		const FVector TargetPosition(X, Y, Z);

		const FVector CurrentBonePos =
			PoseableMeshComponent->GetBoneLocationByName(Name, EBoneSpaces::WorldSpace);

		const FVector NewPos = FMath::Lerp(CurrentBonePos, TargetPosition, Alpha);

		PoseableMeshComponent->SetBoneLocationByName(
			Name,
			NewPos,
			EBoneSpaces::WorldSpace);
	}
	
	// const TArray<FCPoseLandmark>& Landmarks = PoseReceiver->LastLandmarks;
	// if (Landmarks.Num() != 33)
	// 	return;
	//
	// // Pelvis midpoint (root)
	// FCPoseLandmark PelvisLm;
	// PelvisLm.X = (Landmarks[23].X + Landmarks[24].X) * 0.5f;
	// PelvisLm.Y = (Landmarks[23].Y + Landmarks[24].Y) * 0.5f;
	// PelvisLm.Z = (Landmarks[23].Z + Landmarks[24].Z) * 0.5f;
	//
	// // Shoulder width in normalized units
	// float ShoulderWidth = FVector2D(Landmarks[11].X - Landmarks[12].X, Landmarks[11].Y - Landmarks[12].Y).Size();
	// float ScaleCm = DesiredShoulderWidthCM / FMath::Max(ShoulderWidth, 0.01f);
	// ScaleCm = FMath::Lerp(PrevScaleCM, ScaleCm, 0.2f);
	// PrevScaleCM = ScaleCm;
	//
	// for (const TPair<int32, FName>& It: LandmarkToBone)
	// {
	// 	const int32 Index = It.Key;
	// 	const FName BoneName = It.Value;
	// 	
	// 	if (!Landmarks.IsValidIndex(Index) || !Points.Contains(BoneName))
	// 		continue;
	// 	
	// 	const FVector MeshSpacePos = ConvertOne(Landmarks[Index], PelvisLm, ScaleCm, bMirrorY);
	// 	
	// 	PoseableMeshComponent->SetBoneLocationByName(BoneName, MeshSpacePos, EBoneSpaces::ComponentSpace);
	//
	// 	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("%s: %s"), *BoneName.ToString(), *MeshSpacePos.ToString()));
	// 	DrawDebugPoint(
	// 		GetWorld(),
	// 		PoseableMeshComponent->GetComponentTransform().TransformPosition(MeshSpacePos),
	// 		6.f,
	// 		FColor::Cyan,
	// 		false,
	// 		1.0f
	// 	);
	// }
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

