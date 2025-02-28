#pragma once


#include "CoreMinimal.h"
#include "Containers/CircularQueue.h"

extern "C"
{
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/mem.h"
#include "libswscale/swscale.h"
#include "libavutil/file.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/opt.h"
#include "libavutil/avutil.h"
#include "libavutil/time.h"
#include "libavutil/error.h"
#include "libswresample/swresample.h"
}

#include "FFmpegDecoder.generated.h"

#define FFeng_FFMPEG_SAMPLE_RATE 48000

USTRUCT(BlueprintType)
struct FDecoderBeginArgs {

	GENERATED_USTRUCT_BODY()


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FFengFFMPEG")
	FString url;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FFengFFMPEG")
	bool enable_video;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FFengFFMPEG")
	bool enable_audio;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FFengFFMPEG")
	bool loop;
};



class FDecoderRunnable;
class UFFmpegAudioPlayComponent;
UCLASS()
class FFENGFFMPEG_API UFFmpegDecoder : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "FFengFFMPEG", meta = (DisplayName = "开始解码会话"))
	static UFFmpegDecoder* BeginDecodeSession(FDecoderBeginArgs args);
	UFUNCTION(BlueprintCallable, Category = "FFengFFMPEG", meta = (DisplayName = "更新视频帧"))
	bool UpdateFrame();
	UFUNCTION(BlueprintCallable, Category = "FFengFFMPEG", meta = (DisplayName = "结束解码会话"))
	void DestroyDecoder();
	UFUNCTION(BlueprintPure, Category = "FFengFFMPEG", meta = (DisplayName = "是否正在解码"))
	bool IsRunning();
	UFUNCTION(BlueprintPure, Category = "FFengFFMPEG", meta = (DisplayName = "获取输出纹理"))
	UTexture2D* GetTexture();
	UFUNCTION(BlueprintCallable, Category = "FFengFFMPEG", meta = (DisplayName = "创建音频播放组件"))
	UFFmpegAudioPlayComponent* CreateAudioComponent(AActor* parent);

public:
	UFFmpegDecoder();


	bool Initialize(FString url, bool enable_video, bool enable_audio);

protected:
	bool OpenVideo();
	bool OpenAudio();

	void Poll();
	bool Reset();

	void EndPIEHandle(const bool i);

	virtual void BeginDestroy() override;

	FDelegateHandle EndPIEDelegateHandle;

protected:
	double residue_time;

	AVFormatContext* pFormatCtx;

	uint32 video_stream_index;
	int video_width;
	int video_height;
	AVCodecContext* video_codec_ctx;
	AVCodec* video_codec;
	double video_frame_rate;
	double video_starttime;
	double video_basetime;
	bool video_enable;
	TCircularQueue<void*> video_frame_queue = TCircularQueue<void*>(5);
	TCircularQueue<void*> video_frame_queue_free = TCircularQueue<void*>(5);
	double video_current_pts;

	uint32 audio_stream_index;
	AVCodecContext* audio_codec_ctx;
	AVCodec* audio_codec;
	double audio_starttime;
	double audio_basetime;
	bool audio_enable;
	TCircularQueue<float> audio_frame_queue = TCircularQueue<float>(FFeng_FFMPEG_SAMPLE_RATE * 2 / 10);
	double audio_current_pts;

	SwsContext* sws_ctx;
	int video_buffer_size;
	UPROPERTY()
	UTexture2D* texture;

	TSharedPtr<FRunnableThread> decode_thread;
	TSharedPtr<FDecoderRunnable> decode_runnable;

	bool running;
	bool inited;

	bool loop;

	UPROPERTY()
	UFFmpegAudioPlayComponent* audio_component;

	friend class FDecoderRunnable;
	friend class UFFmpegAudioPlayComponent;
};

class FDecoderRunnable : public FRunnable
{
public:
	FDecoderRunnable(UFFmpegDecoder* decoder);

	virtual uint32 Run() override;
	virtual void Stop() override;

protected:
	UFFmpegDecoder* decoder;
};