#include "stdafx.h"
#include "zukiRecognizer.h"

zukiRecognizer::zukiRecognizer()
{
}

void zukiRecognizer::RecognizerMain(cv::Mat & matOutput, rs2::pipeline & pipeline, rs2::spatial_filter & filterSpat, rs2::temporal_filter & filterTemp, rs2_intrinsics & intrinsics)
{
	rs2::align alignTo(RS2_STREAM_COLOR);
	rs2::frameset data = pipeline.wait_for_frames();
	rs2::frameset alignedFrame = alignTo.process(data);

	cv::Mat matColor = funcFormat::frame2Mat(alignedFrame.get_color_frame());

	//cv::Mat matInfrared = funcFormat::frame2Mat(alignedFrame.get_infrared_frame());
	cv::Mat matInfrared = funcFormat::frame2Mat(data.get_infrared_frame());
	
	rs2::colorizer colorize;
	colorize.set_option(RS2_OPTION_COLOR_SCHEME, 0);
	rs2::depth_frame depth = alignedFrame.get_depth_frame();
	depth = filterSpat.process(depth);
	depth = filterTemp.process(depth);
	rs2::frame depthColor = colorize(depth);
	cv::Mat matDepth = funcFormat::frame2Mat(depthColor);

	matOutput = matColor.clone();
}


