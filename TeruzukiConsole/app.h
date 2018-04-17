#pragma once
#ifndef APP_H
#define APP_H

#include <librealsense2\rs.hpp>
#include <opencv2\opencv.hpp>

#include <omp.h>
#include <ctime>
#include <sstream>
#include <vector>
#include <algorithm>

#include "configCamera.h"
#include "funcFormat.h"

#include "zukiRecognizer.h"

typedef enum appState
{
	APPSTATE_EXIT,
	APPSTATE_RECOGNIZER,
} appState;

class app
{
public:
	// application main process
	app(std::string title);
	void cameraInitial();
	void cameraProcess();

	// application config settings
	void setResolution(int stream, int width, int height, int fps);
	void setVisualPreset(std::string preset);

	appState state = APPSTATE_EXIT;
	cv::Mat matOutput;
	cv::Mat matRGB;
	cv::Mat matInfrared;
	cv::Mat matDepth;

private:
	// realsense private parameters
	rs2::pipeline pipeline;
	rs2_intrinsics intrinsics;
	rs2::spatial_filter filterSpat;
	rs2::temporal_filter filterTemp;

	// application private parameters
	std::string windowTitle = "Teruzuki";
	std::string visualPreset = "High Density";
	std::clock_t begin;
	std::clock_t end;
	double elapsed = 0;
	double elapsedAvg = 0;
	int ColorWidth = 640;
	int ColorHeight = 480;
	int ColorFPS = 30;
	int DepthWidth = 640;
	int DepthHeight = 480;
	int DepthFPS = 30;

	// application events
	static void eventMouseS(int event, int x, int y, int flags, void* userdata);
	void eventMouse(int event, int x, int y, int flags);
};

#endif