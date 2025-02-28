#include "FFmpegDecoder.h"
#include "Engine/Texture2D.h"
#include "FFmpegAudioPlayComponent.h"


#if WITH_EDITOR
#include "Editor.h"
#endif // WITH_EDITOR

#undef UpdateResource

UFFmpegDecoder::UFFmpegDecoder() :
	pFormatCtx(nullptr),
	video_stream_index(0),
	video_width(0),
	video_height(0),
	video_codec_ctx(nullptr),
	video_codec(nullptr),
	video_frame_rate(0),
	video_starttime(0),
	video_enable(false),
	video_current_pts(0),
	audio_stream_index(0),
	audio_codec_ctx(nullptr),
	audio_codec(nullptr),
	audio_starttime(0),
	audio_enable(false),
	audio_current_pts(0),
	residue_time(0),

	decode_thread(nullptr),
	decode_runnable(nullptr),
	running(false),
	inited(false),
	loop(false),
	texture(nullptr),
	audio_component(nullptr)
{
}


bool UFFmpegDecoder::Initialize(FString url, bool enable_video, bool enable_audio)
{
	if (inited) {
		return false;
	}
	inited = true;
	av_register_all();


	if (avformat_open_input(&pFormatCtx, TCHAR_TO_UTF8(*url), NULL, NULL)) {
		return false;
	}
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
		return false;
	}

	video_enable = enable_video && OpenVideo();
	audio_enable = enable_audio && OpenAudio();

	// 启动解码线程

	running = true;
	decode_runnable = MakeShareable(new FDecoderRunnable(this));
	decode_thread = MakeShareable(FRunnableThread::Create(decode_runnable.Get(), TEXT("FFengFFmpegDecoder")));

	this->AddToRoot();
#if WITH_EDITOR
	EndPIEDelegateHandle = FEditorDelegates::PrePIEEnded.AddUObject(this, &UFFmpegDecoder::EndPIEHandle);
#endif // WITH_EDITOR

	return true;
}

void UFFmpegDecoder::DestroyDecoder()
{
	if (decode_thread != nullptr)
	{
		decode_runnable->Stop();
		decode_thread->WaitForCompletion();
		decode_thread = nullptr;
	}

	if (video_codec_ctx != nullptr)
	{
		avcodec_close(video_codec_ctx);
		video_codec_ctx = nullptr;
	}
	if (audio_codec_ctx != nullptr)
	{
		avcodec_close(audio_codec_ctx);
		audio_codec_ctx = nullptr;
	}
	if (pFormatCtx != nullptr)
	{
		avformat_close_input(&pFormatCtx);
		pFormatCtx = nullptr;
	}
	void* frame;
	while (video_frame_queue.Dequeue(frame)) {
		av_free(frame);
	}
	while (video_frame_queue_free.Dequeue(frame)) {
		av_free(frame);
	}
	if (sws_ctx != nullptr)
	{
		sws_freeContext(sws_ctx);
		sws_ctx = nullptr;
	}

	if (texture != nullptr)
	{
		texture->ConditionalBeginDestroy();
		texture = nullptr;
		RemoveFromRoot();
	}
	if (audio_component != nullptr)
	{
		audio_component->ConditionalBeginDestroy();
		audio_component = nullptr;
	}
#if WITH_EDITOR
	FEditorDelegates::PrePIEEnded.Remove(EndPIEDelegateHandle);
#endif // WITH_EDITOR

}

bool UFFmpegDecoder::IsRunning()
{
	return running;
}

UFFmpegDecoder* UFFmpegDecoder::BeginDecodeSession(FDecoderBeginArgs args)
{
	UFFmpegDecoder* decoder = NewObject<UFFmpegDecoder>();
	decoder->loop = args.loop;
	if (decoder->Initialize(args.url, args.enable_video, args.enable_audio))
	{
		return decoder;
	}
	else
	{
		decoder->DestroyDecoder();
		return nullptr;
	}
}

bool UFFmpegDecoder::UpdateFrame()
{
	void* framebuffer = nullptr;
	if (!video_frame_queue.Dequeue(framebuffer)) {
		return false;
	}

	FTexture2DRHIRef RHITexture = texture->GetResource()->GetTexture2DRHI();
	ENQUEUE_RENDER_COMMAND(UpdateTexture)([RHITexture, this, framebuffer](FRHICommandListImmediate& RHICmdList) {
		uint32 DestStride = 0;
		void* TextureData = RHICmdList.LockTexture2D(
			RHITexture, 0, RLM_WriteOnly, DestStride, false
		);

		// 内存直接拷贝（如果Stride匹配）
		if (DestStride == (uint32)(this->video_width * 4)) {
			FMemory::Memcpy(TextureData, framebuffer, this->video_width * this->video_height * 4);
		}
		else {
			// 处理行对齐不一致的情况
			const uint8* SrcRow = (uint8_t*)framebuffer;
			uint8* DestRow = (uint8*)TextureData;
			for (int y = 0; y < this->video_height; ++y) {
				FMemory::Memcpy(DestRow, SrcRow, this->video_width * 4);
				SrcRow += this->video_width * 4;
				DestRow += DestStride;
			}
		}
		RHICmdList.UnlockTexture2D(RHITexture, 0, false);
		video_frame_queue_free.Enqueue(framebuffer);
		});

	return false;

}

UTexture2D* UFFmpegDecoder::GetTexture()
{
	return texture;
}

UFFmpegAudioPlayComponent* UFFmpegDecoder::CreateAudioComponent(AActor* parent)
{
	if (audio_component != nullptr)
	{
		audio_component->ConditionalBeginDestroy();
	}
	audio_component = NewObject<UFFmpegAudioPlayComponent>(parent);
	audio_component->setDecoder(this);

	parent->AddInstanceComponent(audio_component);

	audio_component->Activate(true);


	return audio_component;
}

bool UFFmpegDecoder::OpenVideo()
{
	video_stream_index = -1;
	for (uint32 i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			video_stream_index = i;
			break;
		}
	}
	if (video_stream_index == -1)
	{
		return false;
	}
	// 创建解码器
	video_codec_ctx = pFormatCtx->streams[video_stream_index]->codec;
	video_codec = avcodec_find_decoder(video_codec_ctx->codec_id);

	if (video_codec == NULL)
	{
		return false;
	}

	if (avcodec_open2(video_codec_ctx, video_codec, NULL) < 0) {
		return false;
	}

	video_width = video_codec_ctx->width;
	video_height = video_codec_ctx->height;

	AVStream* stream = pFormatCtx->streams[video_stream_index];
	video_frame_rate = av_q2d(stream->avg_frame_rate);
	video_basetime = av_q2d(stream->time_base);
	video_starttime = stream->start_time != AV_NOPTS_VALUE ? stream->start_time * video_basetime : 0;
	// 转换缓冲区
	sws_ctx = sws_getContext(
		video_width, video_height, video_codec_ctx->pix_fmt,
		video_width, video_height, AV_PIX_FMT_BGRA, SWS_FAST_BILINEAR | SWS_ACCURATE_RND | SWS_BITEXACT, NULL, NULL, NULL);
	video_buffer_size = av_image_get_buffer_size(AV_PIX_FMT_BGRA, video_width, video_height, 64);
	while (!video_frame_queue_free.IsFull()) {
		void* buffer = av_mallocz(video_buffer_size);
		video_frame_queue_free.Enqueue(buffer);
	}

	texture = UTexture2D::CreateTransient(video_width, video_height, PF_B8G8R8A8);
	texture->SRGB = 1;
	texture->UpdateResource();


	return true;
}

bool UFFmpegDecoder::OpenAudio()
{
	audio_stream_index = -1;
	for (uint32 i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audio_stream_index = i;
			break;
		}
	}
	if (audio_stream_index == -1)
	{
		return false;
	}
	// 创建解码器
	audio_codec_ctx = pFormatCtx->streams[audio_stream_index]->codec;
	audio_codec_ctx->request_channel_layout = AV_CH_LAYOUT_STEREO;
	audio_codec_ctx->request_sample_fmt = AV_SAMPLE_FMT_FLTP;
	audio_codec_ctx->sample_rate = FFeng_FFMPEG_SAMPLE_RATE;
	audio_codec_ctx->channels = 2;

	audio_codec = avcodec_find_decoder(audio_codec_ctx->codec_id);
	if (audio_codec == NULL)
	{
		return false;
	}
	if (avcodec_open2(audio_codec_ctx, audio_codec, NULL) < 0)
	{
		return false;
	}

	AVStream* stream = pFormatCtx->streams[audio_stream_index];
	audio_basetime = av_q2d(stream->time_base);
	audio_starttime = stream->start_time != AV_NOPTS_VALUE ? stream->start_time * audio_basetime : 0;

	return true;

}

void UFFmpegDecoder::Poll()
{
	AVPacket packet;

	double lasttime = FPlatformTime::Seconds();

	Reset();

	while (running) {
		const double master_clock = audio_enable ? audio_current_pts : video_current_pts;
		double now = FPlatformTime::Seconds();
		residue_time += now - lasttime;
		lasttime = now;

		if (residue_time < 0)
		{
			FPlatformProcess::Sleep(-residue_time);
			continue;
		}

		if (av_read_frame(pFormatCtx, &packet) < 0) {
			if (!loop || !Reset()) {
				break;
			}
			continue;
		}
		const int stream_index = packet.stream_index;
		const bool is_video = (stream_index == video_stream_index) && video_enable;
		const bool is_audio = (stream_index == audio_stream_index) && audio_enable;

		if (!is_video && !is_audio) {
			av_packet_unref(&packet);
			continue;
		}

		// 时间戳转换
		double pts = (packet.pts == AV_NOPTS_VALUE) ? packet.dts : packet.pts;
		pts *= is_video ? video_basetime : audio_basetime;

		// 发送数据包到解码器
		AVCodecContext* codec_ctx = is_video ? video_codec_ctx : audio_codec_ctx;
		int ret = avcodec_send_packet(codec_ctx, &packet);
		if (ret < 0 && ret != AVERROR(EAGAIN)) {
			av_packet_unref(&packet);
			continue;
		}

		// 视频解码处理
		if (is_video) {
			AVFrame* frame = av_frame_alloc();
			while (avcodec_receive_frame(video_codec_ctx, frame) >= 0) {
				auto new_pts = frame->pts * video_basetime;
				residue_time -= new_pts - video_current_pts;
				video_current_pts = frame->pts * video_basetime;

				void* frame_buffer;
				if (video_frame_queue_free.Dequeue(frame_buffer)) {
					// 转换图像格式
					uint8_t* dstData[1] = { (uint8_t*)frame_buffer };
					int dstLinesize[1] = { frame->width * 4 };
					sws_scale(sws_ctx, frame->data, frame->linesize, 0, video_height, dstData, dstLinesize);

					// 将帧送入渲染队列
					video_frame_queue.Enqueue(frame_buffer);
				}
				av_frame_free(&frame);

				frame = av_frame_alloc();
			}
			av_frame_free(&frame);
		}

		// 音频解码处理
		if (is_audio) {
			AVFrame* frame = av_frame_alloc();
			while (avcodec_receive_frame(audio_codec_ctx, frame) >= 0) {

				auto new_pts = frame->pts * audio_basetime;

				if (!video_enable) {
					residue_time -= new_pts - audio_current_pts;
				}

				audio_current_pts = new_pts;

				const int NumSamples = frame->nb_samples * frame->channels;

				float* Left = (float*)frame->data[0];
				float* Right = (float*)frame->data[1];

				for (int i = 0; i < frame->nb_samples; ++i)
				{
					audio_frame_queue.Enqueue(Left[i]);
					audio_frame_queue.Enqueue(Right[i]);
				}

				av_frame_free(&frame);
				frame = av_frame_alloc();
			}
			av_frame_free(&frame);
		}
		av_packet_unref(&packet);
	}
	running = false;

}

bool UFFmpegDecoder::Reset()
{
	if (av_seek_frame(pFormatCtx, -1, 0, AVSEEK_FLAG_BACKWARD) < 0) {
		return false;
	}

	if (video_codec_ctx != nullptr)
	{
		avcodec_flush_buffers(video_codec_ctx);
		video_starttime = 0;
	}
	if (audio_codec_ctx != nullptr)
	{
		avcodec_flush_buffers(audio_codec_ctx);
		video_starttime = 0;
	}
	audio_current_pts = audio_starttime;
	video_current_pts = video_starttime;
	return true;
}

void UFFmpegDecoder::EndPIEHandle(const bool i)
{
	this->DestroyDecoder();
}

void UFFmpegDecoder::BeginDestroy()
{
	Super::BeginDestroy();

	DestroyDecoder();
}

FDecoderRunnable::FDecoderRunnable(UFFmpegDecoder* decoder) :
	decoder(decoder)
{

}

uint32 FDecoderRunnable::Run()
{
	decoder->Poll();
	return 0;
}

void FDecoderRunnable::Stop()
{
	decoder->running = false;
}
