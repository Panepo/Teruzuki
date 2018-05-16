#pragma once
#ifndef ZUKI_RECOGNIZER_H
#define ZUKI_RECOGNIZER_H

#include <librealsense2/rs.hpp>
#include <opencv2\opencv.hpp>

#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/pvl.hpp"

#include "funcStream.h"
#include "funcGeometry3D.h"

#define recogColorFace	cv::Scalar(255, 255, 255)	// white
#define recogColorEye	cv::Scalar(255, 0, 0)		// blue
#define recogColorMouth cv::Scalar(255, 0, 0)		// blue
#define recogColorBlink cv::Scalar(255, 0, 0)		// blue
#define recogFont		cv::FONT_HERSHEY_PLAIN
#define recogColorSmile cv::Scalar(255, 0, 0)		// blue
#define recogColorRecog cv::Scalar(0, 255, 0)		// green

#define recogLengthMin	5
#define recogLengthMax	12

typedef enum RecognizerState
{
	RECOGNIZERSTATE_WAIT,
	RECOGNIZERSTATE_PROCESS,
} RecognizerState;

class configRecognizer
{
public:
	RecognizerState state = RECOGNIZERSTATE_PROCESS;
	cv::Point pixelMouse = cv::Point(0, 0);
	std::string infoText = "";
};


class zukiRecognizer
{
public:
	zukiRecognizer();
	void recognizerMain(cv::Mat & matOutput, rs2::depth_frame & depth, rs2_intrinsics & intrinsics, configZoomer & configZoomer);

	void recognizerMouseHandler(int event, int x, int y, int flags, configZoomer & configZoomer);
	void recognizerKeyboardHandler();

	configRecognizer config;
private:
	void recognizerProcess(cv::Mat & matOutput, rs2::depth_frame & depth, rs2_intrinsics & intrinsics, configZoomer & configZoomer);

	void recogDetectorFD(cv::Mat & matOutput);
	void recogDetectorFI(cv::Mat & matOutput, rs2::depth_frame & depth, rs2_intrinsics & intrinsics);
	void recogDetectorFR(cv::Mat & matOutput);

	void recogDrawerFD(cv::Mat & matOutput);
	void recogDrawerFR(cv::Mat & matOutput);

	cv::Mat matGray;
	cv::Ptr<cv::pvl::FaceDetector> pvlFD;
	cv::Ptr<cv::pvl::FaceRecognizer> pvlFR;
	std::vector<cv::pvl::Face> faces;
	std::vector<int>  personIDs;
	std::vector<int>  confidence;

};

#endif
