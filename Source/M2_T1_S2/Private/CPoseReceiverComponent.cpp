// Fill out your copyright notice in the Description page of Project Settings.


#include "M2_T1_S2/Public/CPoseReceiverComponent.h"

#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"   
#include "Interfaces/IPv4/IPv4Address.h"


UCPoseReceiverComponent::UCPoseReceiverComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}


void UCPoseReceiverComponent::BeginPlay()
{
	Super::BeginPlay();
	CreateSocket();
}

void UCPoseReceiverComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroySocket();
	Super::EndPlay(EndPlayReason);
}


void UCPoseReceiverComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UE_LOG(LogTemp, Log, TEXT("Tick"))
	if (ListenSocket != nullptr)
		ReceiveData();
}


void UCPoseReceiverComponent::CreateSocket()
{
	UE_LOG(LogTemp, Log, TEXT("PoseReceiver: CreateSocket called"));

	if (ListenSocket)
	{
		UE_LOG(LogTemp, Warning, TEXT("PoseReceiver: ListenSocket already exists"));
		return;
	}

	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("PoseReceiver: No SocketSubsystem available"));
		return;
	}

	ListenSocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("PoseUDPListener"), false);
	if (!ListenSocket)
	{
		UE_LOG(LogTemp, Error, TEXT("PoseReceiver: Failed to create UDP socket"));
		return;
	}

	TSharedRef<FInternetAddr> InternetAddr = SocketSubsystem->CreateInternetAddr();

	bool bIsValid = false;
	InternetAddr->SetIp(*IPAddress, bIsValid);
	InternetAddr->SetPort(Port);

	if (!bIsValid)
	{
		UE_LOG(LogTemp, Error, TEXT("PoseReceiver: Invalid Listen IP '%s'"), *IPAddress);
		DestroySocket();
		return;
	}

	const bool bBound = ListenSocket->Bind(*InternetAddr);
	if (!bBound)
	{
		UE_LOG(LogTemp, Error, TEXT("PoseReceiver: Failed to bind UDP socket to %s:%d"), *IPAddress, Port);
		DestroySocket();
		return;
	}

	ListenSocket->SetNonBlocking(true);
	ListenSocket->SetReuseAddr(true);

	UE_LOG(LogTemp, Log, TEXT("PoseReceiver: Listening on %s:%d"), *IPAddress, Port);
}

void UCPoseReceiverComponent::DestroySocket()
{
	if (ListenSocket != nullptr)
	{
		ListenSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenSocket);
		ListenSocket = nullptr;
	}
}

void UCPoseReceiverComponent::ReceiveData()
{
	UE_LOG(LogTemp, Log, TEXT("Waah"))
	uint32 PendingSize = 0;
	while(ListenSocket->HasPendingData(PendingSize))
	{
		TArray<uint8> Buffer;
		Buffer.SetNumUninitialized(FMath::Min(PendingSize, 65507u));
		
		UE_LOG(LogTemp, Warning, TEXT("Loop"))

		int32 BytesRead = 0;
		TSharedRef<FInternetAddr> Sender = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
		if (ListenSocket->RecvFrom(Buffer.GetData(), Buffer.Num(), BytesRead, *Sender))
		{
			UE_LOG(LogTemp, Error, TEXT("Hello"))
			FString JsonString = FString(ANSI_TO_TCHAR(reinterpret_cast<const char*>(Buffer.GetData())));

			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
			TSharedPtr<FJsonObject> RootObj;

			if (FJsonSerializer::Deserialize(Reader, RootObj) && RootObj.IsValid())
			{
				const TArray<TSharedPtr<FJsonValue>>* LandmarksArray;
				if (RootObj->TryGetArrayField(TEXT("landmarks"), LandmarksArray))
				{
					TArray<FCPoseLandmark> NewLandmarks;
					NewLandmarks.Reserve(LandmarksArray->Num());

					for (const TSharedPtr<FJsonValue>& Value : *LandmarksArray)
					{
						TSharedPtr<FJsonObject> LandmarkObj = Value->AsObject();
						if (!LandmarkObj.IsValid())
							continue;

						FCPoseLandmark Lm;
						Lm.X = LandmarkObj->GetNumberField(TEXT("x"));
						Lm.Y = LandmarkObj->GetNumberField(TEXT("y"));
						Lm.Z = LandmarkObj->GetNumberField(TEXT("z"));
						NewLandmarks.Add(Lm);
					}

					LastLandmarks = MoveTemp(NewLandmarks);
					GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Received %d landmarks"), LastLandmarks.Num()));
				}
			}
		}
	}
}
