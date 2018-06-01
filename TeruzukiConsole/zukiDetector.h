#pragma once
#ifndef ZUKI_DETECTOR_H
#define ZUKI_DETECTOR_H

#include <librealsense2/rs.hpp>
#include <opencv2\opencv.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include <fstream>
#include <sstream>

#include "funcStream.h"

#define detectorFont	cv::FONT_HERSHEY_SIMPLEX

typedef enum detectorState
{
	DETECTORSTATE_WAIT,
	DETECTORSTATE_PROCESS,
} detectorState;

class configDetector
{
public:
	detectorState state = DETECTORSTATE_PROCESS;
	cv::Point pixelMouse = cv::Point(0, 0);
	std::string infoText = "";
};

class zukiDetector
{
public:
	zukiDetector();
	void detectorMain(cv::Mat & matOutput, rs2::depth_frame & depth, rs2_intrinsics & intrinsics, configZoomer & configZoomer);
  
	void detectorMouseHandler(int event, int x, int y, int flags, configZoomer & configZoomer);
	void detectorKeyboardHandler(stream & stream);

	configDetector config;

	void setInputSize(int width, int height);
private:
	void detectorProcess(cv::Mat & matOutput, rs2::depth_frame & depth, rs2_intrinsics & intrinsics, configZoomer & configZoomer);

	std::vector<std::string> classes;
	std::vector<cv::Vec3b> colors;
	cv::dnn::Net net;
	float confidenceThreshold = 0.8f;
	const char* classNames[21] = { "background",
		"aeroplane", "bicycle", "bird", "boat",
		"bottle", "bus", "car", "cat", "chair",
		"cow", "diningtable", "dog", "horse",
		"motorbike", "person", "pottedplant",
		"sheep", "sofa", "train", "tvmonitor" };

	const size_t inWidth = 300;
	const size_t inHeight = 300;
	const float WHRatio = inWidth / (float)inHeight;
	const float inScaleFactor = 0.007843f;
	const float meanVal = 127.5;
	cv::Rect crop;
};

#endif
