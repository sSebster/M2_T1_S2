// Fill out your copyright notice in the Description page of Project Settings.


#include "CPoseApplier.h"


UCPoseApplier::UCPoseApplier()
{
	PrimaryComponentTick.bCanEverTick = true;

}


void UCPoseApplier::BeginPlay()
{
	Super::BeginPlay();

}


// Called every frame
void UCPoseApplier::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

