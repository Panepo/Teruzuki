#pragma once
#ifndef ZUKI_RECOGNIZER_H
#define ZUKI_RECOGNIZER_H

#include <librealsense2/rs.hpp>
#include <opencv2\opencv.hpp>

#include "funcFormat.h"

class zukiRecognizer
{
public:
	zukiRecognizer();

	void RecognizerMain(
		cv::Mat & matOutput,
		rs2::pipeline & pipeline, 
		rs2::spatial_filter & filterSpat, 
		rs2::temporal_filter & filterTemp,
		rs2_intrinsics & intrinsics
	);
private:
};

#endif
