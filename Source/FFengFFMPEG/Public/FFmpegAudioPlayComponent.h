#pragma once

#include "CoreMinimal.h"

#include "Components/SynthComponent.h"
#include "Containers/CircularQueue.h"
#include "FFmpegAudioPlayComponent.generated.h"

class UFFmpegDecoder;

UCLASS()
class UFFmpegAudioPlayComponent : public USynthComponent
{
	GENERATED_BODY()

public:
	UFFmpegAudioPlayComponent(const FObjectInitializer& ObjectInitializer);

	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;
	virtual bool ShouldActivate() const override;

	void setDecoder(UFFmpegDecoder* inDecoder);
protected:
	UPROPERTY()
	UFFmpegDecoder* decoder;
private:
};