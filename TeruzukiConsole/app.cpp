#include "stdafx.h"
#include "app.h"

// =================================================================================
// Application main process
// =================================================================================

app::app(std::string title)
{
	windowTitle = title;
}

void app::cameraInitial()
{
	supportDevice device = detectDevice();
	
	state = APPSTATE_RECOGNIZER;
	rs2::config config;
	
	if (device == REALSENSE_415)
	{
		config.enable_stream(RS2_STREAM_COLOR, ColorWidth, ColorHeight, RS2_FORMAT_BGR8, ColorFPS);
		config.enable_stream(RS2_STREAM_INFRARED, DepthWidth, DepthHeight, RS2_FORMAT_BGR8, DepthFPS);
		config.enable_stream(RS2_STREAM_DEPTH, DepthWidth, DepthHeight, RS2_FORMAT_Z16, DepthFPS);
	}
	else if (device == REALSENSE_435)
	{
		config.enable_stream(RS2_STREAM_COLOR, ColorWidth, ColorHeight, RS2_FORMAT_BGR8, ColorFPS);
		config.enable_stream(RS2_STREAM_INFRARED, DepthWidth, DepthHeight, RS2_FORMAT_Y8, DepthFPS);
		config.enable_stream(RS2_STREAM_DEPTH, DepthWidth, DepthHeight, RS2_FORMAT_Z16, DepthFPS);
	}
	
	rs2::pipeline_profile cfg = pipeline.start(config);

	auto sensor = cfg.get_device().first<rs2::depth_sensor>();

	auto range = sensor.get_option_range(RS2_OPTION_VISUAL_PRESET);
	for (auto i = range.min; i < range.max; i += range.step)
		if (std::string(sensor.get_option_value_description(RS2_OPTION_VISUAL_PRESET, i)) == visualPreset)
			sensor.set_option(RS2_OPTION_VISUAL_PRESET, i);

	auto profile = cfg.get_stream(RS2_STREAM_COLOR).as<rs2::video_stream_profile>();
	intrinsics = profile.get_intrinsics();

	filterSpat.set_option(RS2_OPTION_HOLES_FILL, 5);

	for (int i = 0; i < 10; i++) pipeline.wait_for_frames();

	cv::setMouseCallback(windowTitle, eventMouseS, this);
}

void app::cameraProcess()
{
	begin = clock();

	// declare application plugins
	zukiRecognizer recognizer;

	// application main process
	switch (state)
	{
	case APPSTATE_RECOGNIZER:
		recognizer.RecognizerMain(matOutput, pipeline, filterSpat, filterTemp, intrinsics);
		break;
	default:
		state = APPSTATE_EXIT;
		break;
	}

	end = clock();
	elapsed = double(end - begin) * 1000 / CLOCKS_PER_SEC;
}

// =================================================================================
// Application settings
// =================================================================================

void app::setResolution(int stream, int width, int height, int fps)
{
	switch (stream)
	{
	case RS2_STREAM_COLOR:
		ColorWidth = width;
		ColorHeight = height;
		ColorFPS = fps;
		break;
	case RS2_STREAM_INFRARED:
	case RS2_STREAM_DEPTH:
		DepthWidth = width;
		DepthHeight = height;
		DepthFPS = fps;
		break;
	default:
		break;
	}
}

void app::setVisualPreset(std::string preset)
{
	visualPreset = preset;
}

// =================================================================================
// Application events
// =================================================================================

void app::eventMouseS(int event, int x, int y, int flags, void* userdata)
{
	app* temp = reinterpret_cast<app*>(userdata);
	temp->eventMouse(event, x, y, flags);
}

void app::eventMouse(int event, int x, int y, int flags)
{

}
