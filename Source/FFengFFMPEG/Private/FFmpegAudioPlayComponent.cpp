#include "FFmpegAudioPlayComponent.h"

#include "FFmpegDecoder.h"

UFFmpegAudioPlayComponent::UFFmpegAudioPlayComponent(const FObjectInitializer& ObjectInitializer) :
	USynthComponent(ObjectInitializer),
	decoder(nullptr)
{
	bAutoActivate = true;
	NumChannels = 2;
}

int32 UFFmpegAudioPlayComponent::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	if (!decoder) {
		return 0;
	}
	auto& que = decoder->audio_frame_queue;

	int32 i = 0;
	while (i < NumSamples && que.Dequeue(OutAudio[i])) {
		i++;
	}
	if (i < NumSamples)
	{
		FMemory::Memset(OutAudio + i, 0, (NumSamples - i) * sizeof(float));
	}
	return NumSamples;
}

bool UFFmpegAudioPlayComponent::ShouldActivate() const
{
	return decoder && decoder->audio_enable;
}

void UFFmpegAudioPlayComponent::setDecoder(UFFmpegDecoder* inDecoder)
{
	decoder = inDecoder;
}
