#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>
#include <librealsense2/rs.hpp>
#include <librealsense2\rs_advanced_mode.hpp>
#include <librealsense2/rsutil.h>
#include <chrono>

#define ESC_KEY 27
#define SPACE_KEY 32

int main() try
{
	cv::FileStorage fs("../Calibration/output_camera_data_0421025752.yml", cv::FileStorage::READ);
	if (!fs.isOpened())
	{
		std::cout << "File can not be opened. \n";
		return -1;
	}

	// --- open yml file for camera calibration
	int imgWidth = (int)fs["image_width"], imgHeight = (int)fs["image_height"];
	imgWidth = 640;
	imgHeight = 480;
	cv::Size imgSize(imgWidth, imgHeight);
	cv::Mat cameraMat, distCoeffs;
	fs["camera_matrix"] >> cameraMat;
	fs["distortion_error"] >> distCoeffs;
	fs.release();

	// ---  create board object
	int markersX = 5, markersY = 7;
	float markerLength = 0.027;
	float markerSeparationLength = 0.0025;
	cv::Ptr<cv::aruco::Dictionary> markerDict = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);
	cv::Ptr<cv::aruco::GridBoard> gridboard =
		cv::aruco::GridBoard::create(markersX, markersY, markerLength, markerSeparationLength, markerDict);
	cv::Ptr<cv::aruco::Board> board = gridboard.staticCast<cv::aruco::Board>();


	// --- Use RealSense 435
	// Declare RealSense pipeline
	rs2::pipeline pipe;

	// Pipeline settings
	rs2::config rsCfg;
	rsCfg.disable_all_streams();
	rsCfg.enable_stream(RS2_STREAM_COLOR, imgWidth, imgHeight, RS2_FORMAT_BGR8, 30);

	// start
	pipe.start(rsCfg);
	std::chrono::system_clock::time_point start, end;
	while (true)
	{
		start = std::chrono::system_clock::now();

		// get frame	
		rs2::frameset frames;
		if (pipe.poll_for_frames(&frames))
		{
			
		}
		else
		{
			continue;
		}

		//rs2::frameset frames = pipe.wait_for_frames();
		rs2::frame color = frames.get_color_frame();

		// from frame, make Mat 
		cv::Mat frame(imgSize, CV_8UC3, (void*)color.get_data(), cv::Mat::AUTO_STEP);
		cv::Mat frameTmp = frame.clone();
		/*
		std::vector<int> ids;
		std::vector<std::vector<cv::Point2f> > corners;
		cv::aruco::detectMarkers(frame, markerDict, corners, ids);
		// if at least one marker detected
		if (ids.size() > 0) {
			cv::aruco::drawDetectedMarkers(frameTmp, corners, ids);
			cv::Vec3d rvec, tvec;
			int valid = estimatePoseBoard(corners, ids, board, cameraMat, distCoeffs, rvec, tvec);
			// if at least one board marker detected
			if (valid > 0)
				cv::aruco::drawAxis(frameTmp, cameraMat, distCoeffs, rvec, tvec, 0.1);

			cv::imshow("window", frameTmp);
			double fps = 60 * CLOCKS_PER_SEC / (static_cast<double>(clock()) * 1000 - start * 1000);
			printf("%lf fps\n", fps);
			int key = cv::waitKey(1);
			if (key == ESC_KEY) break;
		}
		*/
		end = std::chrono::system_clock::now();
		double fps = 1000000.0 / (static_cast<double> (std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()));
		printf("%lf fps\n", fps);
		cv::imshow("window", frameTmp);
		int key = cv::waitKey(1);
		if (key == ESC_KEY) break;
	}
	pipe.stop();
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