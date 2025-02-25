#include "Rtmp.h"

#include "xop/HttpFlvServer.h"
#include "xop/RtmpPublisher.h"
#include "xop/RtmpClient.h"
#include "xop/HttpFlvServer.h"
#include "xop/H264Parser.h"
#include "net/EventLoop.h"

void URtmpServer::BeginDestroy()
{
	Super::BeginDestroy();
	StopRtmpServer();
}

URtmpServer::URtmpServer() :
	running(false),
	rtmpServer(nullptr),
	eventLoop(nullptr)
{
}


bool URtmpServer::StartRtmpServer(FString host, int port)
{
	if (running) {
		return false;
	}
	eventLoop = MakeShared< xop::EventLoop>(1);
	rtmpServer = xop::RtmpServer::Create(eventLoop.Get());
	rtmpServer->SetChunkSize(60000);
	rtmpServer->SetEventCallback(
		[this](std::string type, std::string stream_path) {
			onLog.Broadcast(FString(type.c_str()), FString(stream_path.c_str()));
		}
	);
	std::string hostStr = TCHAR_TO_UTF8(*host);
	return running = rtmpServer->Start(hostStr, port);
}

bool URtmpServer::StopRtmpServer()
{
	if (running) {
		rtmpServer->Stop();
		eventLoop->Quit();
		running = false;
		return true;
	}
	return false;
}

bool URtmpServer::IsRunning()
{
	return running;
}

FString URtmpServer::GetLocalStreamUrl()
{
	return running ? ("rtmp://127.0.0.1:" + FString::FromInt(rtmpServer->GetPort()) + "/live/stream") : "";
}

URtmpServer* URtmpServer::CreateRtmpServer(FString host, int port, bool& ok)
{
	auto obj = NewObject<URtmpServer>();
	ok = obj->StartRtmpServer(host, port);
	return obj;
}

