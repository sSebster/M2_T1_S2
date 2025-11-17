// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CPoseLandmark.h"
#include "Components/ActorComponent.h"
#include "CPoseReceiverComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class M2_T1_S2_API UCPoseReceiverComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UDP")
	FString IPAddress = TEXT("127.0.0.1");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UDP")
	int32 Port = 12004;

	UPROPERTY(BlueprintReadOnly, Category="UDP")
	TArray<FCPoseLandmark> LastLandmarks;
	
public:
	UCPoseReceiverComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	FSocket* ListenSocket = nullptr;

	void CreateSocket();
	void DestroySocket();
	void ReceiveData();
};
