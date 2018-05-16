#include "stdafx.h"
#include "funcGeometry3D.h"

namespace funcGeometry3D
{
	float calcDist3D(float pixelA[2], float pixelB[2], const rs2::depth_frame * depth, const rs2_intrinsics * intrin)
	{
		float pointA[3] = { 0, 0, 0 };
		float pointB[3] = { 0, 0, 0 };

		float distA = depth->get_distance((int)pixelA[0], (int)pixelA[1]);
		float distB = depth->get_distance((int)pixelB[0], (int)pixelB[1]);

		rs2_deproject_pixel_to_point(pointA, intrin, pixelA, distA);
		rs2_deproject_pixel_to_point(pointB, intrin, pixelB, distB);

		return floor(sqrt(pow(pointA[0] - pointB[0], 2) +
			pow(pointA[1] - pointB[1], 2) +
			pow(pointA[2] - pointB[2], 2)) * 10000) / 100;
	}

	double calcArea3D(std::vector<cv::Point> & contours, cv::Mat* input, const rs2::depth_frame* depth, const rs2_intrinsics* intrin, cv::Point location, cv::Scalar lineColor, int lineSize, configZoomer & configZoomer)
	{
		cv::RotatedRect boundingBox = cv::minAreaRect(contours);
		cv::Point2f corners[4];
		boundingBox.points(corners);

		cv::Point pixelA1 = cv::Point((int)(corners[0].x + corners[1].x) / 2, (int)(corners[0].y + corners[1].y) / 2);
		cv::Point pixelA2 = cv::Point((int)(corners[2].x + corners[3].x) / 2, (int)(corners[2].y + corners[3].y) / 2);
		cv::Point pixelB1 = cv::Point((int)(corners[0].x + corners[3].x) / 2, (int)(corners[0].y + corners[3].y) / 2);
		cv::Point pixelB2 = cv::Point((int)(corners[1].x + corners[2].x) / 2, (int)(corners[1].y + corners[2].y) / 2);
		cv::line(*input, pixelA1, pixelA2, lineColor, lineSize);
		cv::line(*input, pixelB1, pixelB2, lineColor, lineSize);

		pixelA1.x += location.x;
		pixelA1.y += location.y;
		pixelA2.x += location.x;
		pixelA2.y += location.y;
		pixelB1.x += location.x;
		pixelB1.y += location.y;
		pixelB2.x += location.x;
		pixelB2.y += location.y;

		cv::Point pixelA1t, pixelA2t, pixelB1t, pixelB2t;
		funcStream::streamZoomPixelTrans(pixelA1, pixelA1t, configZoomer);
		funcStream::streamZoomPixelTrans(pixelA2, pixelA2t, configZoomer);
		funcStream::streamZoomPixelTrans(pixelB1, pixelB1t, configZoomer);
		funcStream::streamZoomPixelTrans(pixelB2, pixelB2t, configZoomer);

		float a1[2] = { (float)pixelA1t.x, (float)pixelA1t.y };
		float a2[2] = { (float)pixelA2t.x, (float)pixelA2t.y };
		float b1[2] = { (float)pixelB1t.x, (float)pixelB1t.y };
		float b2[2] = { (float)pixelB2t.x, (float)pixelB2t.y };

		float distA = calcDist3D(a1, a2, depth, intrin);
		float distB = calcDist3D(b1, b2, depth, intrin);
		
		std::vector<cv::Point> contourBox;
		for (int i = 0; i < 4; i += 1)
			contourBox.push_back(corners[i]);

		double areaA = cv::contourArea(contours);
		double areaB = cv::contourArea(contourBox);
		
		double area = distA * distB * areaA / areaB;

		return floor(area * 100) / 100;
		
	}
}
