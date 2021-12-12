#include "Overlay.h"
#include <mutex>

Overlay::Overlay()
{
	memset(live_info_, 0, sizeof(live_info_));
	memset(&rect_, 0, sizeof(SDL_Rect));

	snprintf(live_info_[EVENT_TYPE_RTMP_PUSHER].pusher_url, sizeof(LiveInfo::pusher_url), "%s", "rtmp://127.0.0.1:1935/live/test");

	snprintf(encoder_bitrate_kbps_, sizeof(encoder_bitrate_kbps_), "%s", "8000");
	snprintf(encoder_framerate_, sizeof(encoder_framerate_), "%s", "25");
}

Overlay::~Overlay()
{
	Destroy();
}

void Overlay::RegisterObserver(OverlayCallack* callback)
{
	callback_ = callback;
}

bool Overlay::Init(SDL_Window* window, IDirect3DDevice9* device)
{
	Init();

	if (gl_context_ || device_) {
		Destroy();
	}

	if (device) {
		ImGui_ImplSDL2_InitForD3D(window);
		ImGui_ImplDX9_Init(reinterpret_cast<IDirect3DDevice9*>(device));
		window_ = window;
		device_ = (IDirect3DDevice9 *)device;
		return true;
	}

	return false;
}

bool Overlay::Init(SDL_Window* window, SDL_GLContext gl_context)
{
	Init();

	if (gl_context_ || device_) {
		Destroy();
	}

	if (gl_context) {
		static std::once_flag init_flag;
		std::call_once(init_flag, [=]() {
			glfwInit();
			gl3wInit();
		});

		const char* glsl_version = "#version 130";
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

		ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
		ImGui_ImplOpenGL3_Init(glsl_version);

		auto fonts = ImGui::GetIO().Fonts;
		fonts->AddFontFromFileTTF(
			"c:/windows/fonts/msyh.ttc",
			14.0f,
			NULL,
			fonts->GetGlyphRangesChineseSimplifiedCommon()
		);

		window_ = window;
		gl_context_ = gl_context;
		return true;
	}

	return false;
}

void Overlay::SetRect(int x, int y, int w, int h)
{
	rect_.x = x;
	rect_.y = y;
	rect_.w = w;
	rect_.h = h;
}

void Overlay::Destroy()
{
	window_ = nullptr;
	memset(&rect_, 0, sizeof(SDL_Rect));

	if (device_) {
		device_ = nullptr;
		ImGui_ImplDX9_Shutdown();
		ImGui_ImplSDL2_Shutdown();
	}

	if (gl_context_) {
		gl_context_ = nullptr;
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL2_Shutdown();
	}
}

bool Overlay::Begin()
{
	if ((!device_&&!gl_context_) || !window_) {
		return false;
	}

	if (device_) {
		ImGui_ImplDX9_NewFrame();
	}
	else if (gl_context_) {
		ImGui_ImplOpenGL3_NewFrame();
	}
	
	ImGui_ImplSDL2_NewFrame(window_);
	ImGui::NewFrame();
	return true;
}

bool Overlay::End()
{
	if ((!device_ && !gl_context_) || !window_) {
		return false;
	}

	ImGui::EndFrame();
	ImGui::Render();

	if (device_) {
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
	}
	else if (gl_context_) {
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	return true;
}

bool Overlay::Render()
{
	if (!Begin()) {
		return false;
	}

	Copy();
	End();
	return true;
}

void Overlay::Process(SDL_Event* event)
{
	ImGui_ImplSDL2_ProcessEvent(event);
}

void Overlay::Init()
{
	static std::once_flag init_flag;
	std::call_once(init_flag, [=]() {
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
		ImGui::GetStyle().AntiAliasedLines = false;
		ImGui::GetStyle().WindowRounding = 0;
	});
}

bool Overlay::Copy()
{
	if (!rect_.w || !rect_.h) {
		return false;
	}

	int widget_flag = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | 
		ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

	ImGui::SetNextWindowPos(ImVec2((float)rect_.x, (float)rect_.y), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2((float)rect_.w * 3 / 4, (float)rect_.h), ImGuiCond_Always);
	ImGui::Begin("screen-live-setting-widget", nullptr, widget_flag);

	int input_flag = 0;
	float start_x = 10.0, start_y = 20.0;

	/* Encoder selection */
	int encoder_index = 0;
	ImGui::SetCursorPos(ImVec2(start_x, start_y));
	ImGui::Text(u8"����: "); //ImGui::SameLine();
	ImGui::SetCursorPos(ImVec2(start_x + 35, start_y - 3));
	ImGui::RadioButton(u8"����", &encoder_index_, 1); ImGui::SameLine(0, 10);
	ImGui::RadioButton(u8"����", &encoder_index_, 2); ImGui::SameLine(0, 10);
	ImGui::RadioButton(u8"����", &encoder_index_, 3);

	ImGui::SetCursorPos(ImVec2(start_x + 205, start_y ));
	ImGui::Text(u8"ˢ����:");
	ImGui::SetNextItemWidth(25);
	ImGui::SetCursorPos(ImVec2(start_x + 245, start_y - 1));
	ImGui::InputText("##encoder-framerate", encoder_framerate_, sizeof(encoder_framerate_), ImGuiInputTextFlags_CharsDecimal);
	ImGui::SameLine(0, 10);
	ImGui::SetCursorPos(ImVec2(start_x + 280, start_y - 3));
	ImGui::Text(u8"����(kbps):");
	ImGui::SetNextItemWidth(50);
	ImGui::SetCursorPos(ImVec2(start_x + 345, start_y - 1));
	ImGui::InputText("##encoder-bitrate", encoder_bitrate_kbps_, sizeof(encoder_bitrate_kbps_), ImGuiInputTextFlags_CharsDecimal);


	/* RTMP Pusher setting */
	LiveInfo& rtmp_pusher_info = live_info_[EVENT_TYPE_RTMP_PUSHER];
	ImGui::SetCursorPos(ImVec2(start_x, start_y + 40));
	ImGui::Text(u8"��ַ:");  //ImGui::SameLine();
	ImGui::SetNextItemWidth(300);
	ImGui::SetCursorPos(ImVec2(start_x + 35, start_y + 38));
	ImGui::InputText("##rtmp-pusher-url", rtmp_pusher_info.pusher_url, sizeof(LiveInfo::pusher_url), input_flag); ImGui::SameLine(0, 10);
	if (ImGui::Button(!rtmp_pusher_info.state ? u8"��ʼ##rtmp-pusher" : u8"ֹͣ##rtmp-pusher", ImVec2(start_x + 30, start_y))) {
		NotifyEvent(EVENT_TYPE_RTMP_PUSHER);
	} 
	ImGui::SameLine(0, 10);
	ImGui::Text("%s", rtmp_pusher_info.state_info);

	ImGui::End();

	ImGui::SetNextWindowPos(ImVec2((float)rect_.x + (float)rect_.w * 3/4, (float)rect_.y), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2((float)rect_.w/4, (float)rect_.h), ImGuiCond_Always);
	ImGui::Begin("screen-live-info-widget", nullptr, widget_flag);
	ImGui::Text("%s", debug_info_text_.c_str());
	ImGui::End();
	return true;
}

void Overlay::SetLiveState(int event_type, bool state)
{
	live_info_[event_type].state = state;
	memset(live_info_[event_type].state_info, 0, sizeof(LiveInfo::state_info));
}

void Overlay::SetDebugInfo(std::string text)
{
	debug_info_text_ = text;
}

void Overlay::NotifyEvent(int event_type)
{	
	bool *state = nullptr;
	int live_index = -1;
	std::vector<std::string> encoder_settings(3);
	std::vector<std::string> live_settings(1);

	if (!callback_) {
		return;
	}

	encoder_settings[0] = std::string("x264");
	if(encoder_index_ == 2) {
		encoder_settings[0] = std::string("h264_nvenc");
	}
	else if (encoder_index_ == 3) {
		encoder_settings[0] = std::string("h264_qsv");
	}

	encoder_settings[1] = std::string(encoder_framerate_);
	encoder_settings[2] = std::string(encoder_bitrate_kbps_);

	live_index = EVENT_TYPE_RTMP_PUSHER;
	LiveInfo& rtmp_pusher_info = live_info_[live_index];
	rtmp_pusher_info.state = !rtmp_pusher_info.state;
	live_settings[0] = std::string(rtmp_pusher_info.pusher_url);
	state = &rtmp_pusher_info.state;

	
	if (state && live_index>=0) {
		if (*state) {
			*state = callback_->StartLive(event_type, encoder_settings, live_settings);
			if (!*state) {
				snprintf(live_info_[live_index].state_info, sizeof(LiveInfo::state_info), "%s", "failed");
			}
			//else {
				//snprintf(live_info_[live_index].state_info, sizeof(LiveInfo::state_info), "%s", "succeeded");
			//}
		}
		else {
			callback_->StopLive(event_type);
			memset(live_info_[live_index].state_info, 0, sizeof(LiveInfo::state_info));
		}
	}
}