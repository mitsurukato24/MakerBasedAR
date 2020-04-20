#include <iostream>
#include <opencv2/opencv.hpp>
#include <librealsense2/rs.hpp>

#define ESC_KEY 27
#define SPACE_KEY 32

int main() try
{
	// Use RealSense 435
	rs2::log_to_console(RS2_LOG_SEVERITY_ERROR);
	
	// Declare RealSense pipeline
	rs2::pipeline pipe;

	// Pipeline settings
	rs2::config rsCfg;
	rsCfg.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_BGR8, 30);  // from my laptop 30fps is the highest

	// start
	pipe.start(rsCfg);
	while (true)
	{
		// get frame
		rs2::frameset frames = pipe.wait_for_frames();
		rs2::frame color = frames.get_color_frame();

		// from frame, make Mat 
		cv::Mat frame(cv::Size(640, 480), CV_8UC3, (void*)color.get_data(), cv::Mat::AUTO_STEP);

		cv::imshow("window", frame);
		int key = cv::waitKey(1);
		if (key == ESC_KEY)
		{
			break;
		}
	}

	cv::destroyAllWindows();
	return EXIT_SUCCESS;
}
catch (const rs2::error &e)
{
	std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
	return EXIT_FAILURE;
}
catch (const std::exception& e)
{
	std::cerr << e.what() << std::endl;
	return EXIT_FAILURE;
}