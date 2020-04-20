#include <iostream>
#include <opencv2//opencv.hpp>

#define ESC_KEY 27
#define SPACE_KEY 32

int main() try
{
	cv::Mat m;
	// Task 14
	/*
	cv::VideoCapture cap(0);
	if (!cap.isOpened()) return -1;

	cv::FileStorage fs("../Task12/output_camera_data_0415234738.yml", cv::FileStorage::READ);
	if (!fs.isOpened())
	{
		std::cout << "File can not be opened. \n";
		return -1;
	}

	// use Logicool C910
	// open yml file
	cap.set(cv::CAP_PROP_AUTOFOCUS, 0);
	cap.set(cv::CAP_PROP_FPS, 30);
	cv::Size imgSize((int)fs["image_width"], (int)fs["image_height"]);
	cap.set(cv::CAP_PROP_FRAME_WIDTH, imgSize.width);
	cap.set(cv::CAP_PROP_FRAME_HEIGHT, imgSize.height);
	cap.set(cv::CAP_PROP_FOCUS, (int)fs["camera_focus_parameter"]);  // 0~255

	double chessSize = (int)fs["cell_size"];
	cv::Size boardSize((int)fs["board_width"], (int)fs["board_height"]);
	cv::Mat cameraMat, distCoeff;
	fs["camera_matrix"] >> cameraMat;
	fs["distortion_error"] >> distCoeff;
	fs.release();

	std::vector<cv::Point3f> objPts;  // true 3d coordinates of corners
	for (int h = 0; h < boardSize.height; h++)
	{
		for (int w = 0; w < boardSize.width; w++)
		{
			objPts.push_back(cv::Point3f(chessSize*w, chessSize*h, 0));
		}
	}
	std::vector<cv::Point3f> axis = {
		cv::Point3f(chessSize*(boardSize.width - 1), 0, 0),
		cv::Point3f(0, chessSize*(boardSize.height - 1), 0),
		cv::Point3f(0, 0, -chessSize*(boardSize.height - 1))
	};

	cv::Mat frame, grayFrame;
	int chessBoardFlags = cv::CALIB_CB_FAST_CHECK | cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE;
	cv::Mat rvec, tvec;
	while (true)
	{
		cap >> frame;
		std::vector<cv::Point2f> cornerPts;

		// --- find calibration board
		bool found = cv::findChessboardCorners(frame, boardSize, cornerPts, chessBoardFlags);
		if (!found)
		{
			cv::imshow("project xyz axis", frame);
			int key = cv::waitKey(1);
			if (key == ESC_KEY) break;
			continue;
		}

		cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);

		int winSize = 11;  // half of search window
		cv::TermCriteria termCriteia(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.01);
		cv::cornerSubPix(grayFrame, cornerPts, cv::Size(winSize, winSize), cv::Size(-1, -1), termCriteia);

		// --- calculate r, t
		cv::solvePnP(objPts, cornerPts, cameraMat, distCoeff, rvec, tvec);

		// --- project axis
		std::vector<cv::Point2f> axisPtsInImg;
		cv::projectPoints(axis, rvec, tvec, cameraMat, distCoeff, axisPtsInImg);
		cv::line(frame, cornerPts[0], axisPtsInImg[0], cv::Scalar(0, 0, 255), 5);
		cv::line(frame, cornerPts[0], axisPtsInImg[1], cv::Scalar(0, 255, 0), 5);
		cv::line(frame, cornerPts[0], axisPtsInImg[2], cv::Scalar(255, 0, 0), 5);

		cv::imshow("project xyz axis", frame);
		int key = cv::waitKey(1);
		if (key == ESC_KEY) break;
	}
	*/
	return 0;
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