#include "stdafx.h"
#include "app.h"

// =================================================================================
// Application main process
// =================================================================================

app::app(std::string title)
{
	windowTitle = title;
	cv::namedWindow(windowTitle, CV_WINDOW_AUTOSIZE);
	cv::setMouseCallback(windowTitle, eventMouseS, this);
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

	// plugin initial
	detector.setInputSize(ColorWidth, ColorHeight);
}

void app::cameraProcess()
{
	std::string infoText;
	auto t0 = std::chrono::high_resolution_clock::now();
	static double elapsedAvg = 0;
	typedef std::chrono::duration<double, std::ratio<1, 1000>> ms;
	
	// application main process
	//switch (stream)
	//{
	//case STREAM_COLOR:
	//case STREAM_INFRARED:
	//case STREAM_DEPTH:
	//	funcStream::streamSelector(matOutput, depth, stream, pipeline, filterSpat, filterTemp, configZoomer);
	//	break;
	//case STREAM_FILE:
	//	break;
	//default:
	//	break;
	//}

	rs2_stream align = RS2_STREAM_COLOR;
	rs2::depth_frame depth = funcStream::streamSelector(matOutput, stream, pipeline, filterDec, filterSpat, filterTemp, align, configZoomer);

	switch (state)
	{
	case APPSTATE_RECOGNIZER:
		recognizer.recognizerMain(matOutput, depth, intrinsics, configZoomer);
		break;
	case APPSTATE_DETECTOR:
		detector.detectorMain(matOutput, depth, intrinsics, configZoomer);
		break;
	default:
		state = APPSTATE_EXIT;
		break;
	}

	auto t1 = std::chrono::high_resolution_clock::now();
	double elapsed = std::chrono::duration_cast<ms>(t1 - t0).count();
	elapsedAvg = floor((elapsedAvg * 9 + elapsed) / 10);

	std::ostringstream strs;
	strs << elapsedAvg;
	std::string str = strs.str() + " ms " + infoText;
	funcStream::streamInfoer(&matOutput, str);

	eventKeyboard();
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
	int modx, mody;
	
	if (x >= 0 && x <= ColorWidth)
		modx = x;
	else
		modx = ColorWidth - 1;
	if (y >= 0 && y <= ColorHeight)
		mody = y;
	else
		mody = ColorHeight - 1;

	switch (state)
	{
	case APPSTATE_RECOGNIZER:
		recognizer.recognizerMouseHandler(event, modx, mody, flags, configZoomer);
	default:
		break;
	}
}

void app::eventKeyboard()
{
	char key = cv::waitKey(10);

	if (key == 'q' || key == 'Q')
		state = APPSTATE_EXIT;
	else if (key == 'w' || key == 'W')
	{
		time_t t = std::time(nullptr);
		#pragma warning( disable : 4996 )
		tm tm = *std::localtime(&t);

		std::ostringstream oss;
		oss << std::put_time(&tm, "%d-%m-%Y %H-%M-%S");
		std::string str = windowTitle + "_" + oss.str() + ".jpg";
		cv::imwrite(str, matOutput);
		std::cout << "file saved: " << str << std::endl;
	}
	else if (key == 'r' || key == 'R')
	{
		if (state == APPSTATE_RECOGNIZER)
			recognizer.recognizerKeyboardHandler();
		else
		{
			state = APPSTATE_RECOGNIZER;
			configZoomer.miniMap = false;
		}
	}
	else if (key == 'd' || key == 'D')
	{
		if (state == APPSTATE_DETECTOR)
			detector.detectorKeyboardHandler(stream);
		else
		{
			state = APPSTATE_DETECTOR;
			configZoomer.miniMap = false;
		}
	}
	else if (key == 'a' || key == 'A')
	{
		if (state == APPSTATE_RECOGNIZER)
		{
			if (recognizer.config.switchFI)
				recognizer.config.switchFI = false;
			else
				recognizer.config.switchFI = true;
		}
	}
	else if (key == 's' || key == 'S')
	{
		switch (stream)
		{
		case STREAM_COLOR:
			stream = STREAM_INFRARED;
			recognizer.config.state = RECOGNIZERSTATE_WAIT;
			break;
		case STREAM_INFRARED:
			stream = STREAM_DEPTH;
			recognizer.config.state = RECOGNIZERSTATE_WAIT;
			break;
		case STREAM_DEPTH:
			stream = STREAM_COLOR;
			recognizer.config.state = RECOGNIZERSTATE_PROCESS;
			break;
		}
	}
}
