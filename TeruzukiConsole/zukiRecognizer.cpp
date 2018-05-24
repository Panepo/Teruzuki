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
	
	if (config.switchFI)
		recogDetectorFI(matOutput, depth, intrinsics);

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

void zukiRecognizer::recogDetectorFI(cv::Mat & matOutput, rs2::depth_frame & depth, rs2_intrinsics & intrinsics)
{
	cv::Point posEyeL, posEyeR, posMouth;
	bool FIdist = false;
	bool FIdiff = false;
	int eraseCount = 0;
	int size = (int)faces.size();

	std::cout << "===================================================" << std::endl;
	for (int i = 0; i < size; i += 1)
	{
		posEyeL = faces[i].get<cv::Point>(cv::pvl::Face::LEFT_EYE_POS);
		posEyeR = faces[i].get<cv::Point>(cv::pvl::Face::RIGHT_EYE_POS);
		posMouth = faces[i].get<cv::Point>(cv::pvl::Face::MOUTH_POS);

#if enableIdenDist
		FIdist = recogIdentifierDist(posEyeL, posEyeR, posMouth, depth, intrinsics);
		if (!FIdist)
		{
			faces.erase(faces.begin() + i - eraseCount);
			eraseCount += 1;
			std::cout << "Photo detected, face erased. (dist)" << std::endl;
			continue;
		}
#endif

#if enableIdenDiff
		FIdiff = recogIdentifierDiff(posEyeL, posEyeR, posMouth, depth, intrinsics);
		if (!FIdiff)
		{
			faces.erase(faces.begin() + i - eraseCount);
			eraseCount += 1;
			std::cout << "Photo detected, face erased. (diff)" << std::endl;
			continue;
		}
#endif
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

bool zukiRecognizer::recogIdentifierDist(cv::Point posEyeL, cv::Point posEyeR, cv::Point posMouth, rs2::depth_frame & depth, rs2_intrinsics & intrinsics)
{
	float pixelEyeL[2], pixelEyeR[2], pixelMouth[2];
	float distA, distB, distC;
	
	pixelEyeL[0] = (float)posEyeL.x;
	pixelEyeL[1] = (float)posEyeL.y;
	pixelEyeR[0] = (float)posEyeR.x;
	pixelEyeR[1] = (float)posEyeR.y;
	pixelMouth[0] = (float)posMouth.x;
	pixelMouth[1] = (float)posMouth.y;

	distA = funcGeometry3D::calcDist3D(pixelEyeL, pixelEyeR, &depth, &intrinsics);
	distB = funcGeometry3D::calcDist3D(pixelEyeR, pixelMouth, &depth, &intrinsics);
	distC = funcGeometry3D::calcDist3D(pixelEyeL, pixelMouth, &depth, &intrinsics);

	//std::cout << "distA: ";
	//std::cout << distA;
	//std::cout << " distB: ";
	//std::cout << distB;
	//std::cout << " distC: ";
	//std::cout << distC << std::endl;

	if (distA < recogLengthMin || distA > recogLengthMax 
		|| distB < recogLengthMin || distB > recogLengthMax
		|| distC < recogLengthMin || distC > recogLengthMax)
	{
		return false;
	}
	
	return true;
}

bool zukiRecognizer::recogIdentifierDiff(cv::Point posEyeL, cv::Point posEyeR, cv::Point posMouth, rs2::depth_frame & depth, rs2_intrinsics & intrinsics)
{
	bool diff1, diff2, diff3 = false;
	
	diff1 = recogIdentifierPath(posEyeL, posEyeR, depth, intrinsics);
	diff2 = recogIdentifierPath(posMouth, posEyeR, depth, intrinsics);
	diff3 = recogIdentifierPath(posEyeL, posMouth, depth, intrinsics);

	if (diff1 == false || diff2 == false || diff3 == false)
		return false;
	else
		return true;
}

bool zukiRecognizer::recogIdentifierPath(cv::Point start, cv::Point end, rs2::depth_frame & depth, rs2_intrinsics & intrinsics)
{
	float posA[2] = { (float)start.x, (float)start.y };
	float posB[2] = { (float)end.x, (float)end.y };

	float xdiff = abs(posA[0] - posB[0]);
	float ydiff = abs(posA[1] - posB[1]);
	int posX = 0, posY = 0;
	float dist = 0, parm = 1;
	float directX = 1, directY = 1;
	std::vector<float> output;

	// get depth data of the route
	if (posA[0] - posB[0] < 0)
		directX = -1;

	if (posA[1] - posB[1] < 0)
		directY = -1;

	if (xdiff < ydiff)
	{
		for (int i = 0; i < ydiff; i += 1)
		{
			posX = (int)floor(posB[0] + directX * (float)i * xdiff / ydiff);
			posY = (int)floor(posB[1] + directY * (float)i);
			dist = depth.get_distance(posX, posY);
			output.push_back(dist);
		}
	}
	else
	{
		for (int i = 0; i < xdiff; i += 1)
		{
			posX = (int)floor(posB[0] + directX * (float)i);
			posY = (int)floor(posB[1] + directY * (float)i * ydiff / xdiff);
			dist = depth.get_distance(posX, posY);
			output.push_back(dist);
		}
	}

	if (output.size() > 1)
	{
		std::vector<float> outputDiff;
		std::vector<float> outputDiff2;

		for (int i = 0; i < output.size() - 1; i += 1)
		{
			float temp = output[i - 1] - output[i];
			outputDiff.push_back(temp);
		}

		for (int i = 0; i < outputDiff.size() - 1; i += 1)
		{
			float temp = outputDiff[i - 1] - outputDiff[i];
			outputDiff2.push_back(temp);
		}

		auto diffMinMax = std::minmax_element(outputDiff2.begin(), outputDiff2.end());

		if (*diffMinMax.second < recogDiffMin)
			return false;
		else
			return true;
	}

	return false;
}



