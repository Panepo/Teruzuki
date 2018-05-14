#include "stdafx.h"
#include "zukiRecognizer.h"

zukiRecognizer::zukiRecognizer()
{
	pvlFD = cv::pvl::FaceDetector::create();
	pvlFR = cv::pvl::FaceRecognizer::create();

	pvlFD->setTrackingModeEnabled(true);
	pvlFR->setTrackingModeEnabled(true);
}

void zukiRecognizer::recognizerMain(cv::Mat & matOutput, rs2::depth_frame & depth, rs2_intrinsics & intrinsics, configZoomer & configZoomer)
{
	switch (config.state)
	{
	case RECOGNIZERSTATE_WAIT:
		break;
	case RECOGNIZERSTATE_PROCESS:
		recognizerProcess(matOutput, depth, intrinsics, configZoomer);
		break;
	default:
		break;
	}
}

void zukiRecognizer::recognizerMouseHandler(int event, int x, int y, int flags, configZoomer & configZoomer)
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

void zukiRecognizer::recognizerKeyboardHandler()
{
	for (uint i = 0; i < personIDs.size(); i++)
	{
		// If the face is unknown and its ROP angle is less than 23
		if (personIDs[i] == cv::pvl::FACE_RECOGNIZER_UNKNOWN_PERSON_ID && faces[i].get<int>(cv::pvl::Face::ROP_ANGLE) < 23)
		{
			int personID = pvlFR->createNewPersonID();
			pvlFR->registerFace(matGray, faces[i], personID, true);
		}
	}
}

void zukiRecognizer::recognizerProcess(cv::Mat & matOutput, rs2::depth_frame & depth, rs2_intrinsics & intrinsics, configZoomer & configZoomer)
{
	recogDetectorFD(matOutput);
	recogDrawerFD(matOutput);

	recogDetectorFR(matOutput);
	recogDrawerFR(matOutput);
}

void zukiRecognizer::recogDetectorFD(cv::Mat & matOutput)
{
	cv::cvtColor(matOutput, matGray, cv::COLOR_BGR2GRAY);

	//do face detection
	pvlFD->detectFaceRect(matGray, faces);

	for (uint i = 0; i < faces.size(); ++i)
	{
		pvlFD->detectEye(matGray, faces[i]);
		pvlFD->detectMouth(matGray, faces[i]);
		pvlFD->detectBlink(matGray, faces[i]);
		pvlFD->detectSmile(matGray, faces[i]);
	}
}

void zukiRecognizer::recogDetectorFR(cv::Mat & matOutput)
{
	//do face recognition
	if (faces.size() > 0)
	{
		// Keep as much as the maximum number of faces in FR's tracking mode
		if (faces.size() > static_cast<uint>(pvlFR->getMaxFacesInTracking()))
		{
			for (uint i = pvlFR->getMaxFacesInTracking(); i < faces.size(); ++i)
				faces.pop_back();
		}

		pvlFR->recognize(matGray, faces, personIDs, confidence);
	}
}

void zukiRecognizer::recogDrawerFD(cv::Mat & matOutput)
{
	for (uint i = 0; i < faces.size(); ++i)
	{
		const cv::pvl::Face& face = faces[i];
		cv::Rect faceRect = face.get<cv::Rect>(cv::pvl::Face::FACE_RECT);

		// Draw face rect
		cv::rectangle(matOutput, faceRect, recogColorFace, 2);

		// Draw eyes
		cv::circle(matOutput, face.get<cv::Point>(cv::pvl::Face::LEFT_EYE_POS), 3, recogColorEye, 2);
		cv::circle(matOutput, face.get<cv::Point>(cv::pvl::Face::RIGHT_EYE_POS), 3, recogColorEye, 2);

		// Draw mouth
		cv::circle(matOutput, face.get<cv::Point>(cv::pvl::Face::MOUTH_POS), 5, recogColorMouth, 2);

		// Draw blink
#define MARGIN 5
		int left_x = faceRect.x - MARGIN;
		int left_height = faceRect.height / 100 * face.get<int>(cv::pvl::Face::LEFT_BLINK_SCORE);

		int right_x = faceRect.x + faceRect.width + MARGIN;
		int right_height = faceRect.height / 100 * face.get<int>(cv::pvl::Face::RIGHT_BLINK_SCORE);

		int y = faceRect.y + faceRect.height;

		cv::line(matOutput, cv::Point(left_x, y), cv::Point(left_x, y - left_height), recogColorBlink, 4);
		std::stringstream left(std::ios_base::app | std::ios_base::out);
		left << face.get<int>(cv::pvl::Face::LEFT_BLINK_SCORE);
		cv::putText(matOutput, left.str(), cv::Point(left_x - 25, y), recogFont, 1, recogColorBlink, 2);

		cv::line(matOutput, cv::Point(right_x, y), cv::Point(right_x, y - right_height), recogColorBlink, 4);
		std::stringstream right(std::ios_base::app | std::ios_base::out);
		right << face.get<int>(cv::pvl::Face::RIGHT_BLINK_SCORE);
		cv::putText(matOutput, right.str(), cv::Point(right_x + 5, y), recogFont, 1, recogColorBlink, 2);

		// Draw smile
		int x = faceRect.x + 5;
		y = faceRect.y + faceRect.height + 15;

		std::stringstream smile(std::ios_base::app | std::ios_base::out);
		smile << face.get<int>(cv::pvl::Face::SMILE_SCORE);
		cv::putText(matOutput, "smile score", cv::Point(x, y), recogFont, 1, recogColorSmile);
		cv::putText(matOutput, smile.str(), cv::Point(x, y + 20), recogFont, 1, recogColorSmile);
	}
}

void zukiRecognizer::recogDrawerFR(cv::Mat & matOutput)
{
	cv::String str;

	for (uint i = 0; i < faces.size(); i++)
	{
		const cv::pvl::Face& face = faces[i];
		cv::Rect faceRect = face.get<cv::Rect>(cv::pvl::Face::FACE_RECT);

		//draw FR info
		str = (personIDs[i] > 0) ? cv::format("Person: %d(%d)", personIDs[i], confidence[i]) : "UNKNOWN";

		cv::Size strSize = cv::getTextSize(str, recogFont, 1.2, 2, NULL);
		cv::Point strPos(faceRect.x + (faceRect.width / 2) - (strSize.width / 2), faceRect.y - 2);
		cv::putText(matOutput, str, strPos, recogFont, 1.2, recogColorRecog, 2);
	}
}


