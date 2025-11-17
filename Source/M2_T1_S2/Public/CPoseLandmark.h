#pragma once

#include "CPoseLandmark.generated.h"

USTRUCT(BlueprintType)
struct FCPoseLandmark
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	float X = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float Y = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	float Z = 0.0f;
};
