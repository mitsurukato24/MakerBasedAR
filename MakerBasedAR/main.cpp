#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>
#include <chrono>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader.hpp"

#define ESC_KEY 27
#define SPACE_KEY 32

int main()
{
	// --- open yml file for camera calibration
	// use logicool C910
	cv::FileStorage fs("C:/Users/mkkat/Source/Repos/mitsurukato24/ImplementationSkillCheck/ImplementationSkillCheck/Task12/output_camera_data_0420231016.yml", cv::FileStorage::READ);
	if (!fs.isOpened())
	{
		std::cout << "File can not be opened. /n";
		return -1;
	}
	int imgWidth = (int)fs["image_width"], imgHeight = (int)fs["image_height"];
	cv::Size imgSize(imgWidth, imgHeight);
	int focusParam = (int)fs["camera_focus_parameter"];
	cv::Mat camMat, distCoeffs;
	fs["camera_matrix"] >> camMat;
	fs["distortion_error"] >> distCoeffs;
	fs.release();
	
	cv::VideoCapture cap(0);
	cap.set(cv::CAP_PROP_AUTOFOCUS, 0);
	cap.set(cv::CAP_PROP_FOCUS, 15);
	cap.set(cv::CAP_PROP_FRAME_WIDTH, imgWidth);
	cap.set(cv::CAP_PROP_FRAME_HEIGHT, imgHeight);
	cap.set(cv::CAP_PROP_FPS, 30);

	GLFWwindow* window;
	if (!initializeGLAndMakeWindow(window, imgWidth, imgHeight, "Marker-based AR")) return -1;
	// load the shaders of frame
	GLuint frameShaderID = loadShaders("TransformVertexShader.vertexshader", "TextureFragmentShader.fragmentshader");

	// load the shaders of obj
	GLuint objShaderID = loadShaders("StandardShading.vertexshader", "StandardShading.fragmentshader");
	std::vector<glm::vec3> objShadingVertices, objShadingNormals;
	if (!loadObj("bunny.obj", objShadingVertices, objShadingNormals)) return -1;
	
	glm::mat4 glCamPoseMat(1.0f),glModelMat(1.0f), glViewMat(1.0f);
	float maxF = 820.0f, minF = 0.1f, fx = camMat.at<float>(0, 0), fy = camMat.at<float>(1, 1);
	float cx = camMat.at<float>(0, 2), cy = camMat.at<float>(2, 1);
	glm::mat4 glProjMat(
		2.0f * fx / float(imgWidth), 0.0f, 0.0f, 0.0f,
		0.0f, 2.0f * fy / float(imgHeight), 0.0f, 0.0f,
		1.0f - 2.0f*cx / float(imgWidth), -1.0f + 2.0f*cy / float(imgHeight), -(maxF + minF) / (maxF - minF), -1.0f,
		0.0f, 0.0f, -2.0f*maxF*minF / (maxF- minF), 0.0f
	);

	glm::vec3 rotation_axis(1.0f, 0.0f, 0.0f);
	glModelMat = glm::rotate(glm::mat4(),
		glm::radians(89.0f), rotation_axis) *
		glm::scale(glm::mat4(),
			glm::vec3(0.5f, 0.5f, 0.5f));

	// --- start
	// TODO: make threads
	cv::Ptr<cv::aruco::Dictionary> markerDict = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);
	cv::Ptr<cv::aruco::GridBoard> board = cv::aruco::GridBoard::create(5, 7, 0.04, 0.01, markerDict);
	std::chrono::system_clock::time_point start, end;
	cv::Mat frame, frameShow;
	do
	{
		// Clear the screen.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		start = std::chrono::system_clock::now();
		cap >> frame;

		cv::cvtColor(frame, frameShow, cv::COLOR_BGR2RGB);
		cv::flip(frameShow, frameShow, 0);
		drawBackground(frameShow, frameShaderID);
		glClear(GL_DEPTH_BUFFER_BIT);

		std::vector<int> ids;
		std::vector<std::vector<cv::Point2f> > corners;
		cv::aruco::detectMarkers(frame, markerDict, corners, ids);
		// if at least one marker detected
		if (ids.size() > 0) {
			cv::Vec3d rvec, tvec;
			int valid = estimatePoseBoard(corners, ids, board, camMat, distCoeffs, rvec, tvec);
			// if at least one board marker detected
			if (valid > 0)
			{
				// tvec = 0.026 * tvec;
				cv::Mat R(3, 3, CV_32F);
				cv::Rodrigues(rvec, R);
				std::cout << R << std::endl << tvec << std::endl;
				cv::Mat T = (cv::Mat_<float>(4, 4) <<
					R.at<float>(0, 0), -R.at<float>(0, 1), -R.at<float>(0, 2), 0.0f,
					R.at<float>(1, 0), -R.at<float>(1, 1), -R.at<float>(1, 2), 0.0f,
					R.at<float>(2, 0), -R.at<float>(2, 1), -R.at<float>(2, 2), 0.0f,
					float(tvec(0)), -float(tvec(1)), -float(tvec(2)), 1.0f
					);
				glViewMat = glm::make_mat4(reinterpret_cast<GLfloat*>(T.data));

				drawShadingObj(objShadingVertices, objShadingNormals, glModelMat, glViewMat, glProjMat, objShaderID);
			}
		}
		
		end = std::chrono::system_clock::now();
		double fps = 1000000.0 / (static_cast<double> (std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()));
		printf("%lf fps\n", fps);

		// Swap buffers
		glfwSwapBuffers(window);
	} while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);

	glfwTerminate();
	return 0;
}