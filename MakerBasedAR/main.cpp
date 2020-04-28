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
#include "util.hpp"
#define ESC_KEY 27
#define SPACE_KEY 32

void detectMarkersAndDrawCube(
	cv::Mat &img,
	const cv::Mat &camMat, const cv::Mat &distCoeffs,
	const float markerLen,
	const cv::Ptr<cv::aruco::Board> &board,
	const cv::Ptr<cv::aruco::Dictionary> &markerDict,
	const GLuint &backgroundID,
	glm::mat4 &glModelMat, glm::mat4 &glViewMat, glm::mat4 &glProjMat,
	glm::mat4 &glMVPMat, const GLuint &glMVPMatID,
	const GLuint &objID,
	const GLuint &vertexBuffer, const GLuint &colorBuffer
	)
{
	// show background
	cv::Mat imgTmp;
	cv::cvtColor(img, imgTmp, cv::COLOR_BGR2RGB);
	cv::flip(imgTmp, imgTmp, 0);
	drawBackground(imgTmp, backgroundID);
	glClear(GL_DEPTH_BUFFER_BIT);

	// detect markers

	std::vector<int> ids;
	std::vector<std::vector<cv::Point2f> > corners;
	cv::aruco::detectMarkers(img, markerDict, corners, ids);
	if (ids.size() > 0)
	{
		cv::Vec3d rvec, tvec;
		int valid = cv::aruco::estimatePoseBoard(corners, ids, board, camMat, distCoeffs, rvec, tvec);
		if (valid > 0)
		{
			cv::Mat R = cv::Mat::eye(3, 3, CV_64FC1);
			cv::Mat T = cv::Mat::eye(4, 4, CV_64FC1);
			rvec[0] *= -1.0f;
			cv::Rodrigues(rvec, R);
			tvec[1] *= -1.0; tvec[2] *= -1.0;
			T(cv::Rect(0, 0, 3, 3)) = R.t() *1.0;
			tvec /= markerLen;
			T = (cv::Mat_<double>(4, 4) <<
				R.at<double>(0, 0), R.at<double>(0, 1), R.at<double>(0, 2), 0.0f,
				R.at<double>(1, 0), R.at<double>(1, 1), R.at<double>(1, 2), 0.0f,
				R.at<double>(2, 0), R.at<double>(2, 1), R.at<double>(2, 2), 0.0f,
				tvec(0), tvec(1), tvec(2), 1.0f
				);

			glViewMat = glm::make_mat4(reinterpret_cast<GLdouble*>(T.data));
			glMVPMat = glProjMat * glViewMat * glModelMat;
		}
		drawCube(objID, glMVPMatID, glMVPMat, vertexBuffer, colorBuffer);
	}

}


int main()
{
	// --- read camera parameters from .yml file generated when you calibrated
	int imgWidth, imgHeight, focusParam;
	cv::Mat camMat, distCoeffs;
	std::string filename = "output_camera_data.yml";
	if (!getCameraParamsFromYML(filename, imgWidth, imgHeight, focusParam, camMat, distCoeffs)) return -1;;

	// --- gen OpenGL window and initialize
	GLFWwindow* window;
	if (!initializeGLAndMakeWindow(window, imgWidth, imgHeight, "Marker-based AR")) return -1;

	// --- Create and compile our GLSL program from the shaders
	GLuint objID = loadShaders("TransformVertexShader.vertexshader", "ColorFragmentShader.fragmentshader");
	GLuint backgroundID = loadShaders("TransformVertexShader2.vertexshader", "TextureFragmentShader.fragmentshader");

	// --- open web camera and set params
	cv::VideoCapture cap(0);
	if (!cap.isOpened()) return -1;
	cap.set(cv::CAP_PROP_AUTOFOCUS, 0);
	cap.set(cv::CAP_PROP_FOCUS, 15);
	cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
	cap.set(cv::CAP_PROP_FRAME_HEIGHT, 720);
	cap.set(cv::CAP_PROP_FPS, 30);

	// --- Get a handle for our "MVP" uniform
	GLuint glMVPMatID = glGetUniformLocation(objID, "MVP");

	// --- make a model matrix : resize for the board
	float markerLen = 0.027f;
	float markerSep = 0.002f;
	float scale = 1.0f;
	glm::mat4 glModelMat = glm::mat4(1.0f) * glm::scale(glm::mat4(),
		glm::vec3(scale*((markerLen*2.5f+markerSep*2.0f)/markerLen), scale*((markerLen*3.5f + markerSep*3.0f) / markerLen), scale))*glm::translate(glm::mat4(), glm::vec3(1.0f, -1.0f, -1.0f));

	// --- make a view matrix
	glm::mat4 glViewMat(1.0f);

	// --- make a projection matrix from intrinsic parameters
	glm::mat4 glProjMat;

	float maxF = 1000.0f, minF = 0.1f, fx = (float)camMat.at<double>(0, 0), fy = (float)camMat.at<double>(1, 1);
	float cx = (float)camMat.at<double>(0, 2), cy = (float)camMat.at<double>(1, 2);
	glProjMat = glm::mat4(
		2.0f * fx / ((float)imgWidth), 0.0f, 0.0f, 0.0f,
		0.0f, 2.0f * fy / ((float)imgHeight), 0.0f, 0.0f,
		1.0f - 2.0f*cx / ((float)imgWidth), -1.0f + 2.0f*cy / ((float)imgHeight), -(maxF + minF) / (maxF - minF), -1.0f,
		0.0f, 0.0f, -2.0f*maxF*minF / (maxF - minF), 0.0f
	);

	// --- make a mvp matrix
	glm::mat4 glMVPMat = glProjMat * glViewMat * glModelMat;

	// --- object: A cube has 6 faces with 2 triangles each, so this makes 6*2=12 triangles, and 12*3 vertices
	// this is from OpenGL tutorial chapter 4
	// TODO: read .obj file
	static const GLfloat glVertexBufferData[] = {
		-1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f,
		1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		1.0f,-1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f,-1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f,-1.0f, 1.0f
	};
	static const GLfloat glColorBufferData[] = {
		0.583f,  0.771f,  0.014f,
		0.609f,  0.115f,  0.436f,
		0.327f,  0.483f,  0.844f,
		0.822f,  0.569f,  0.201f,
		0.435f,  0.602f,  0.223f,
		0.310f,  0.747f,  0.185f,
		0.597f,  0.770f,  0.761f,
		0.559f,  0.436f,  0.730f,
		0.359f,  0.583f,  0.152f,
		0.483f,  0.596f,  0.789f,
		0.559f,  0.861f,  0.639f,
		0.195f,  0.548f,  0.859f,
		0.014f,  0.184f,  0.576f,
		0.771f,  0.328f,  0.970f,
		0.406f,  0.615f,  0.116f,
		0.676f,  0.977f,  0.133f,
		0.971f,  0.572f,  0.833f,
		0.140f,  0.616f,  0.489f,
		0.997f,  0.513f,  0.064f,
		0.945f,  0.719f,  0.592f,
		0.543f,  0.021f,  0.978f,
		0.279f,  0.317f,  0.505f,
		0.167f,  0.620f,  0.077f,
		0.347f,  0.857f,  0.137f,
		0.055f,  0.953f,  0.042f,
		0.714f,  0.505f,  0.345f,
		0.783f,  0.290f,  0.734f,
		0.722f,  0.645f,  0.174f,
		0.302f,  0.455f,  0.848f,
		0.225f,  0.587f,  0.040f,
		0.517f,  0.713f,  0.338f,
		0.053f,  0.959f,  0.120f,
		0.393f,  0.621f,  0.362f,
		0.673f,  0.211f,  0.457f,
		0.820f,  0.883f,  0.371f,
		0.982f,  0.099f,  0.879f
	};

	GLuint vertexBuffer;
	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glVertexBufferData), glVertexBufferData, GL_STATIC_DRAW);

	GLuint colorBuffer;
	glGenBuffers(1, &colorBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glColorBufferData), glColorBufferData, GL_STATIC_DRAW);

	// --- ready for marker detection
	cv::Ptr<cv::aruco::Dictionary> markerDict = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);
	cv::Ptr<cv::aruco::GridBoard> board = cv::aruco::GridBoard::create(5, 7, markerLen, markerSep, markerDict);

	// --- start roop
	std::chrono::system_clock::time_point start, end;
	do {
		start = std::chrono::system_clock::now();
		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// read a frame
		cv::Mat img;
		cap >> img;

		detectMarkersAndDrawCube(
			img, camMat, distCoeffs, markerLen, board, markerDict, backgroundID,
			glModelMat, glViewMat, glProjMat, glMVPMat, glMVPMatID,
			objID, vertexBuffer, colorBuffer
			);

		// Swap buffers
		glfwSwapBuffers(window);

		end = std::chrono::system_clock::now();
		double fps = 1000000.0 / (static_cast<double> (std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()));
		printf("%lf fps\n", fps);

	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);

	// Cleanup VBO and shader
	glDeleteBuffers(1, &vertexBuffer);
	glDeleteBuffers(1, &colorBuffer);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}