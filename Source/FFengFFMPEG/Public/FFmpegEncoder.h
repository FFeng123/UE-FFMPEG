// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "UnrealClient.h"
#include "CoreMinimal.h"
#include "RHIResources.h"
#include "RHICommandList.h"
#include "ISubmixBufferListener.h"
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
#include "FFmpegEncoder.generated.h"

class ISubmixBufferListener;

class FEncoderThread;

USTRUCT(BlueprintType)
struct FEncoderConfig {
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FFengFFMPEG")
	FString OutFileName = "";
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FFengFFMPEG")
	bool UseGPU = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FFengFFMPEG")
	int VideoFps = 60;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FFengFFMPEG")
	int VideoBitRate = 10 * 1024 * 1024;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FFengFFMPEG")
	float AudioDelay = 0.0f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FFengFFMPEG")
	float SoundVolume = 1.0f;
};

class AudioListener;

UCLASS(Blueprintable)
class FFENGFFMPEG_API UFFmpegEncoder :public UObject
{
	GENERATED_BODY()
public:
	static UFFmpegEncoder* BeginEncodeSessionByRenderTarget(UObject* WorldContextObject, FRenderTarget* texture, FEncoderConfig cfg);
	UFUNCTION(BlueprintCallable, Category = "FFengFFMPEG", meta = (DisplayName = "从视口目标启动编码会话", WorldContext = "WorldContextObject"))
	static UFFmpegEncoder* BeginEncodeSessionByViewport(UObject* WorldContextObject, UGameViewportClient* viewport, FEncoderConfig cfg);
	UFUNCTION(BlueprintCallable, Category = "FFengFFMPEG", meta = (DisplayName = "从渲染目标纹理启动编码会话", WorldContext = "WorldContextObject"))
	static UFFmpegEncoder* BeginEncodeSessionByURenderTarget(UObject* WorldContextObject, UTextureRenderTarget2D* rt, FEncoderConfig cfg);
	UFUNCTION(BlueprintCallable, Category = "FFengFFMPEG", meta = (DisplayName = "从默认视口目标启动编码会话", WorldContext = "WorldContextObject"))
	static UFFmpegEncoder* BeginEncodeSessionByDefaultViewport(UObject* WorldContextObject, FEncoderConfig cfg);

	UFUNCTION(BlueprintCallable, Category = "FFengFFMPEG", meta = (DisplayName = "结束编码会话"))
	bool EndEncodeSession();
	UFUNCTION(BlueprintCallable, Category = "FFengFFMPEG", meta = (DisplayName = "编码视频Tick"))
	void GetScreenVideoData();
public:
	UFFmpegEncoder();
	virtual ~UFFmpegEncoder();
	bool Initialize_Director_RenderTarget(UWorld* World, FRenderTarget* RenderTarget, FString OutFileName, bool UseGPU, int VideoFps, int VideoBitRate, float AudioDelay, float SoundVolume);
	bool Initialize_Director_Base(FString OutFileName, bool UseGPU, int VideoFps, int VideoBitRate, float AudioDelay, float SoundVolume, uint32 width, uint32 height);

	//void Begin_Receive_VideoData();


	//void OnBackBufferReady_RenderThread(SWindow& SlateWindow, const FTexture2DRHIRef& BackBuffer);
	void DestoryDirector();

protected:
	void Encode_Video_Frame(uint8_t* rgb);
	void Encode_SetCurrentAudioTime(uint8_t* rgb);
	void Encode_Audio_Frame(uint8_t* rgb);
	void Encode_Finish();

	void Begin_Receive_AudioData();
	bool AddTickTime(float time);

	void EndWindowReader(const bool i);
	void EndWindowReader_StandardGame(void* i);

	bool Create_Video_Encoder(bool is_use_NGPU, const char* out_file_name, int bit_rate);
	bool Create_Audio_Encoder(const char* audioencoder_name);
	void Video_Frame_YUV_From_BGR(uint8_t* rgb);
	void Create_Audio_Swr();

	void AddTickFunction();
	void AddEndFunction();
	void CreateEncodeThread();
	void Set_Audio_Volume(AVFrame* frame);

	bool Alloc_Video_Filter();
	uint32 FormatSize_X(uint32 x);



	// 重写此函数实现不同格式的数据转换到BGR24
	virtual uint8_t* ConvertToBGR24(uint8_t* data);
	// 重写此函数实现不同的视频数据源
	virtual void GetScreenVideoDataImpl();

protected:
	bool IsDestory = false;
	FString filter_descr;

	int video_fps;
	uint32 Video_Frame_Duration;
	float Video_Tick_Time;
	double CurrentAuidoTime = 0.0;
	float audio_delay;
	float audio_volume;
	uint32 width;
	uint32 height;
	uint32 out_width;
	uint32 out_height;

	//FTexture2DRHIRef GameTexture;

	// 管道使用
	void* pipe_io_buffer;
	void* hPipe;
	//


	AVFilterInOut* outputs;
	AVFilterInOut* inputs;
	AVFilterGraph* filter_graph;
	AVFilterContext* buffersink_ctx;
	AVFilterContext* buffersrc_ctx;

	FAudioDevice* AudioDevice;
	TArray<FColor> TexturePixel;
	float ticktime = 0.0f;
	int64_t video_pts = 0;
	uint8_t* buff_bgr;
	int32_t video_index;
	int32_t audio_index;

	FEncoderThread* Runnable;
	FRunnableThread* RunnableThread;

	AVFormatContext* out_format_context;
	AVCodecContext* video_encoder_codec_context;
	AVCodecContext* audio_encoder_codec_context;

	SwsContext* sws_context;
	AVStream* out_video_stream;
	AVStream* out_audio_stream;
	SwrContext* swr;
	uint8_t* outs[2];

	FTSTicker::FDelegateHandle TickDelegateHandle;
	FDelegateHandle EndPIEDelegateHandle;

	AVFrame* audio_frame;
	AVFrame* video_frame;

	uint32 LolStride;

	UPROPERTY()
	UWorld* world;
	FRenderTarget* renderTarget;


	TSharedRef< AudioListener> AudioListenerPtr = MakeShared<AudioListener>(this);
	friend class AudioListener;
};
class AudioListener :public ISubmixBufferListener {
private:
	UFFmpegEncoder* encoder;
public:
	AudioListener(UFFmpegEncoder* encoder);
	virtual void OnNewSubmixBuffer(const USoundSubmix* OwningSubmix, float* AudioData, int32 NumSamples, int32 NumChannels, const int32 SampleRate, double AudioClock) override;

};
