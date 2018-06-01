#include "stdafx.h"
#include "zukiDetector.h"

// =================================================================================
// Plugin main process
// =================================================================================

zukiDetector::zukiDetector()
{
	net = cv::dnn::readNetFromCaffe("./source/MobileNetSSD_deploy.prototxt", "./source/MobileNetSSD_deploy.caffemodel");

	// computation backends
	// "0: default C++ backend
	// "1: Halide language (http://halide-lang.org/)
	// "2: Intel's Deep Learning Inference Engine (https://software.seek.intel.com/deep-learning-deployment)}
	net.setPreferableBackend(0);
	
	// target computation devices
	// "0: CPU target (by default)
	// "1: OpenCL
	net.setPreferableTarget(0);

	setInputSize(640, 480);
}

void zukiDetector::detectorMain(cv::Mat & matOutput, rs2::depth_frame & depth, rs2_intrinsics & intrinsics, configZoomer & configZoomer)
{
	switch (config.state)
	{
	case DETECTORSTATE_WAIT:
		break;
	case DETECTORSTATE_PROCESS:
		detectorProcess(matOutput, depth, intrinsics, configZoomer);
		break;
	default:
		break;
	}
}

// =================================================================================
// Plugin events
// =================================================================================

void zukiDetector::detectorMouseHandler(int event, int x, int y, int flags, configZoomer & configZoomer)
{
	int value;

	switch (event)
	{
	case CV_EVENT_MOUSEMOVE:
		config.pixelMouse.x = x;
		config.pixelMouse.y = y;
		break;
	case CV_EVENT_MOUSEWHEEL:
		configZoomer.pixelZoom.x = x;
		configZoomer.pixelZoom.y = y;

		value = cv::getMouseWheelDelta(flags);
		if (value > 0 && configZoomer.scaleZoom < zoomerScaleMax)
			configZoomer.scaleZoom += (float) 0.1;
		else if (value < 0 && configZoomer.scaleZoom > zoomerScaleMin)
			configZoomer.scaleZoom -= (float) 0.1;
		break;
	default:
		break;
	}
}

void zukiDetector::detectorKeyboardHandler(stream & stream)
{
	switch (stream)
	{
	case STREAM_COLOR:
		stream = STREAM_INFRARED;
		break;
	case STREAM_INFRARED:
		stream = STREAM_DEPTH;
		break;
	case STREAM_DEPTH:
		stream = STREAM_COLOR;
		break;
	}
}

// =================================================================================
// Plugin settings
// =================================================================================

void zukiDetector::setInputSize(int width, int height)
{
	cv::Size cropSize;
	float ratio = width / (float)height;

	if (ratio > WHRatio)
		cropSize = cv::Size(static_cast<int>(height * WHRatio), height);
	else
		cropSize = cv::Size(width, static_cast<int>(width / WHRatio));

	crop = cv::Rect(cv::Point((width - cropSize.width) / 2, (height - cropSize.height) / 2), cropSize);

}

// =================================================================================
// Plugin sub functions
// =================================================================================

void zukiDetector::detectorProcess(cv::Mat & matOutput, rs2::depth_frame & depth, rs2_intrinsics & intrinsics, configZoomer & configZoomer)
{
	cv::Mat inputBlob = cv::dnn::blobFromImage(matOutput, inScaleFactor,
		cv::Size(inWidth, inHeight), meanVal, false);

	net.setInput(inputBlob, "data"); //set the network input
	cv::Mat detection = net.forward("detection_out"); //compute output

	cv::Mat detectionMat(detection.size[2], detection.size[3], CV_32F, detection.ptr<float>());

	cv::Mat depthMat = funcFormat::frame2Mat(depth);

	matOutput = matOutput(crop);
	depthMat = depthMat(crop);

	for (int i = 0; i < detectionMat.rows; i++)
	{
		float confidence = detectionMat.at<float>(i, 2);

		if (confidence > confidenceThreshold)
		{
			size_t objectClass = (size_t)(detectionMat.at<float>(i, 1));

			int xLeftBottom = static_cast<int>(detectionMat.at<float>(i, 3) * matOutput.cols);
			int yLeftBottom = static_cast<int>(detectionMat.at<float>(i, 4) * matOutput.rows);
			int xRightTop = static_cast<int>(detectionMat.at<float>(i, 5) * matOutput.cols);
			int yRightTop = static_cast<int>(detectionMat.at<float>(i, 6) * matOutput.rows);

			cv::Rect object((int)xLeftBottom, (int)yLeftBottom,
				(int)(xRightTop - xLeftBottom),
				(int)(yRightTop - yLeftBottom));

			object = object  & cv::Rect(0, 0, depthMat.cols, depthMat.rows);

			// Calculate mean depth inside the detection region
			// This is a very naive way to estimate objects depth
			// but it is intended to demonstrate how one might 
			// use depht data in general
			cv::Scalar m = mean(depthMat(object));

			std::ostringstream ss;
			ss << classNames[objectClass] << " ";
			ss << floor(m[0]) / 10 << " cm away";
			std::string conf(ss.str());

			rectangle(matOutput, object, cv::Scalar(0, 255, 0));
			int baseLine = 0;
			cv::Size labelSize = cv::getTextSize(ss.str(), detectorFont, 0.5, 1, &baseLine);

			auto center = (object.br() + object.tl())*0.5;
			center.x = center.x - labelSize.width / 2;

			cv::rectangle(matOutput, cv::Rect(cv::Point(center.x, center.y - labelSize.height),
				cv::Size(labelSize.width, labelSize.height + baseLine)),
				cv::Scalar(255, 255, 255), CV_FILLED);
			cv::putText(matOutput, ss.str(), center,
				detectorFont, 0.5, cv::Scalar(0, 0, 0));
		}
	}
}
