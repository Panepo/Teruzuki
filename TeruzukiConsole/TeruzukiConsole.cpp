// TeruzukiConsole.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "app.h"

int main(int argc, char * argv[]) try
{
	std::string appTitle = "Teruzuki";
	app teruzuki(appTitle);

	teruzuki.setResolution(RS2_STREAM_COLOR, 1280, 720, 30);
	teruzuki.setResolution(RS2_STREAM_DEPTH, 640, 480, 30);
	teruzuki.setVisualPreset("High Density");
	teruzuki.cameraInitial();

	cv::namedWindow(appTitle, cv::WINDOW_AUTOSIZE);

	while (teruzuki.state)
	{
		teruzuki.cameraProcess();
		cv::imshow(appTitle, teruzuki.matOutput);
	}

	return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
	std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
	system("pause");
	return EXIT_FAILURE;
}
catch (const std::exception& e)
{
	std::cerr << e.what() << std::endl;
	system("pause");
	return EXIT_FAILURE;
}