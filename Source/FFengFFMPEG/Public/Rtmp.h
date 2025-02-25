#pragma once

#include "CoreMinimal.h"

#include "xop/RtmpServer.h"

#include "Rtmp.generated.h"

UCLASS(Blueprintable)
class URtmpServer : public UObject
{
	GENERATED_BODY()

protected:

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FHandlerLog, FString, type, FString, path);

	TSharedPtr< xop::EventLoop> eventLoop;
	std::shared_ptr< xop::RtmpServer> rtmpServer;
	bool running;
	void BeginDestroy() override;

private:
public:
	URtmpServer();


	UFUNCTION(BlueprintCallable, Category = "FFengFFMPEG|RTMP", meta = (DisplayName = "启动RTMP服务器"))
	bool StartRtmpServer(FString host, int port);

	UFUNCTION(BlueprintCallable, Category = "FFengFFMPEG|RTMP", meta = (DisplayName = "停止RTMP服务器"))
	bool StopRtmpServer();
	UFUNCTION(BlueprintCallable, Category = "FFengFFMPEG|RTMP", meta = (DisplayName = "创建并启动RTMP服务器"))
	static URtmpServer* CreateRtmpServer(FString host, int port, bool& ok);

	UFUNCTION(BlueprintPure, Category = "FFengFFMPEG|RTMP", meta = (DisplayName = "RTMP服务器正在运行"))
	bool IsRunning();
	UFUNCTION(BlueprintPure, Category = "FFengFFMPEG|RTMP", meta = (DisplayName = "RTMP服务器本地推流地址"))
	FString GetLocalStreamUrl();


	UPROPERTY(BlueprintAssignable, Category = "FFengFFMPEG|RTMP", meta = (DisplayName = "事件RTMP日志"))
	FHandlerLog onLog;
};



