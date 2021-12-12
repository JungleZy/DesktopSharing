#ifndef SCREEN_LIVE_H
#define SCREEN_LIVE_H
 
#include "xop/RtspServer.h"
#include "xop/RtspPusher.h"
#include "xop/RtmpPublisher.h"
#include "H264Encoder.h"
#include "ScreenCapture/ScreenCapture.h"
#include <mutex>
#include <atomic>
#include <string>
#include <set>

#define SCREEN_LIVE_RTMP_PUSHER 3

struct AVConfig
{
	uint32_t bitrate_bps = 8000000;
	uint32_t framerate = 25;
	//uint32_t gop = 25;

	std::string codec = "x264"; // [software codec: "x264"]  [hardware codec: "h264_nvenc, h264_qsv"]

	bool operator != (const AVConfig &src) const {
		if (src.bitrate_bps != bitrate_bps || src.framerate != framerate ||
			src.codec != codec) {
			return true;
		}
		return false;
	}
};

struct LiveConfig
{
	// pusher
	std::string rtmp_url;
};

class ScreenLive
{
public:
	ScreenLive & operator=(const ScreenLive &) = delete;
	ScreenLive(const ScreenLive &) = delete;
	static ScreenLive& Instance();
	~ScreenLive();

	bool Init(AVConfig& config);
	void Destroy();
	bool IsInitialized() { return is_initialized_; };

	int StartCapture();
	int StopCapture();

	int StartEncoder(AVConfig& config);
	int StopEncoder();
	bool IsEncoderInitialized() { return is_encoder_started_; };

	bool StartLive(int type, LiveConfig& config);
	void StopLive(int type);
	bool IsConnected(int type);

	bool GetScreenImage(std::vector<uint8_t>& bgra_image, uint32_t& width, uint32_t& height);

	std::string GetStatusInfo();

private:
	ScreenLive();
	
	void EncodeVideo();
	void PushVideo(const uint8_t* data, uint32_t size, uint32_t timestamp);
	bool IsKeyFrame(const uint8_t* data, uint32_t size);

	bool is_initialized_ = false;
	bool is_capture_started_ = false;
	bool is_encoder_started_ = false;

	AVConfig av_config_;
	std::mutex mutex_;

	// capture
	ScreenCapture* screen_capture_ = nullptr;

    // encoder
	H264Encoder h264_encoder_;
	std::shared_ptr<std::thread> encode_video_thread_ = nullptr;

	// streamer
	xop::MediaSessionId media_session_id_ = 0;
	std::unique_ptr<xop::EventLoop> event_loop_ = nullptr;
	std::shared_ptr<xop::RtmpPublisher> rtmp_pusher_ = nullptr;

	// status info
	std::atomic_int encoding_fps_;
};

#endif
