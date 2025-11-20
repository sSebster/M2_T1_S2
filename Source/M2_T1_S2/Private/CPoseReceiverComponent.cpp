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

	if (ListenSocket != nullptr)
		ReceiveData();
}


void UCPoseReceiverComponent::CreateSocket()
{
	UE_LOG(LogTemp, Log, TEXT("PoseReceiver: CreateSocket called"));
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Green, TEXT("CreateSocket called"));

	if (ListenSocket)
	{
		UE_LOG(LogTemp, Warning, TEXT("PoseReceiver: ListenSocket already exists"));
		GEngine->AddOnScreenDebugMessage(1000, 5.f, FColor::Yellow, TEXT("ListenSocket already exists"));
		return;
	}

	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("PoseReceiver: No SocketSubsystem available"));
		GEngine->AddOnScreenDebugMessage(1000, 5.f, FColor::Red, TEXT("No SocketSubsystem available"));
		return;
	}

	ListenSocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("PoseUDPListener"), false);
	if (!ListenSocket)
	{
		UE_LOG(LogTemp, Error, TEXT("PoseReceiver: Failed to create UDP socket"));
		GEngine->AddOnScreenDebugMessage(1000, 5.f, FColor::Red, TEXT("Failed to create UDP socket"));
		return;
	}

	TSharedRef<FInternetAddr> InternetAddr = SocketSubsystem->CreateInternetAddr();

	bool bIsValid = false;
	InternetAddr->SetIp(*IPAddress, bIsValid);
	InternetAddr->SetPort(Port);

	if (!bIsValid)
	{
		UE_LOG(LogTemp, Error, TEXT("PoseReceiver: Invalid Listen IP '%s'"), *IPAddress);
		GEngine->AddOnScreenDebugMessage(1000, 5.f, FColor::Red, FString::Printf(TEXT("Invalid Listen IP '%s'"), *IPAddress));
		DestroySocket();
		return;
	}

	ListenSocket->SetNonBlocking(true);
	ListenSocket->SetReuseAddr(true);

	const bool bBound = ListenSocket->Bind(*InternetAddr);
	if (!bBound)
	{
		const ESocketErrors LastError = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode();
		const TCHAR* ErrorStr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetSocketError(LastError);

		UE_LOG(LogTemp, Error,
			TEXT("PoseReceiver: Failed to bind UDP socket to %s:%d (Error %d: %s)"),
			*IPAddress, Port,
			static_cast<int32>(LastError),
			ErrorStr ? ErrorStr : TEXT("Unknown"));
		
		DestroySocket();
		return;
	}
	
	UE_LOG(LogTemp, Log, TEXT("PoseReceiver: Listening on %s:%d"), *IPAddress, Port);
	GEngine->AddOnScreenDebugMessage(1000, 5.f, FColor::Green, FString::Printf(TEXT("Listening on %s:%d"), *IPAddress, Port));
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
	uint32 PendingSize = 0;
	while(ListenSocket->HasPendingData(PendingSize))
	{
		TArray<uint8> Buffer;
		Buffer.SetNumUninitialized(FMath::Min(PendingSize, 65507u));

		int32 BytesRead = 0;
		TSharedRef<FInternetAddr> Sender = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
		if (ListenSocket->RecvFrom(Buffer.GetData(), Buffer.Num(), BytesRead, *Sender))
		{
			FString String = FString(ANSI_TO_TCHAR(reinterpret_cast<const char*>(Buffer.GetData())));
			LastData = String;
			// UE_LOG(LogTemp, Log, TEXT("Received:  %s"), *String);
			// GEngine->AddOnScreenDebugMessage(1004, 5.f, FColor::Green, FString::Printf(TEXT("Received %d bytes as %s"), BytesRead, *String));
		}
	}
}
