#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "util.hpp"

bool initializeGLAndMakeWindow(GLFWwindow* &window, const int width, const int height, const char* windowName)
{
	// Initialise GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		getchar();
		return false;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //We don't want the old OpenGL 

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(width, height, windowName, NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		getchar();
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return false;
	}

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
	return true;
}

bool getCameraParamsFromYML(const std::string filename, int &imgWidth, int &imgHeight, int &focusParam, cv::Mat &camMat, cv::Mat &distCoeffs)
{
	cv::FileStorage fs(filename, cv::FileStorage::READ);
	if (!fs.isOpened())
	{
		std::cout << "File can not be opened. /n";
		return false;
	}
	imgWidth = (int)fs["image_width"], imgHeight = (int)fs["image_height"];
	focusParam = (int)fs["camera_focus_parameter"];
	cv::Mat tmp1, tmp2;
	fs["camera_matrix"] >> tmp1;
	fs["distortion_error"] >> tmp2;
	fs.release();
	tmp1.reshape(1, 3);
	std::cout << tmp1 << std::endl;
	tmp1.copyTo(camMat);
	tmp2.copyTo(distCoeffs);
	return true;
}

GLuint loadShaders(const char* vertexFilePath, const char* fragmentFilePath)
{
	// Create the shaders
	GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string vertexShaderCode;
	std::ifstream vertexShaderStream(vertexFilePath, std::ios::in);
	if (vertexShaderStream.is_open()) {
		std::stringstream sstr;
		sstr << vertexShaderStream.rdbuf();
		vertexShaderCode = sstr.str();
		vertexShaderStream.close();
	}
	else {
		printf("Impossible to open %s. \n", vertexFilePath);
		getchar();
		return 0;
	}

	// Read the Fragment Shader code from the file
	std::string fragmentShaderCode;
	std::ifstream fragmentShaderStream(fragmentFilePath, std::ios::in);
	if (fragmentShaderStream.is_open()) {
		std::stringstream sstr;
		sstr << fragmentShaderStream.rdbuf();
		fragmentShaderCode = sstr.str();
		fragmentShaderStream.close();
	}

	GLint result = GL_FALSE;
	int infoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertexFilePath);
	char const * vertexSourcePointer = vertexShaderCode.c_str();
	glShaderSource(vertexShaderID, 1, &vertexSourcePointer, NULL);
	glCompileShader(vertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &result);
	glGetShaderiv(vertexShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (infoLogLength > 0) {
		std::vector<char> vertexShaderErrorMessage(infoLogLength + 1);
		glGetShaderInfoLog(vertexShaderID, infoLogLength, NULL, &vertexShaderErrorMessage[0]);
		printf("%s\n", &vertexShaderErrorMessage[0]);
	}

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragmentFilePath);
	char const * fragmentSourcePointer = fragmentShaderCode.c_str();
	glShaderSource(fragmentShaderID, 1, &fragmentSourcePointer, NULL);
	glCompileShader(fragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &result);
	glGetShaderiv(fragmentShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (infoLogLength > 0) {
		std::vector<char> fragmentShaderErrorMessage(infoLogLength + 1);
		glGetShaderInfoLog(fragmentShaderID, infoLogLength, NULL, &fragmentShaderErrorMessage[0]);
		printf("%s\n", &fragmentShaderErrorMessage[0]);
	}

	// Link the program
	printf("Linking program\n");
	GLuint programID = glCreateProgram();
	glAttachShader(programID, vertexShaderID);
	glAttachShader(programID, fragmentShaderID);
	glLinkProgram(programID);

	// Check the program
	glGetProgramiv(programID, GL_LINK_STATUS, &result);
	glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
	if (infoLogLength > 0) {
		std::vector<char> programErrorMessage(infoLogLength + 1);
		glGetProgramInfoLog(programID, infoLogLength, NULL, &programErrorMessage[0]);
		printf("%s\n", &programErrorMessage[0]);
	}

	glDetachShader(programID, vertexShaderID);
	glDetachShader(programID, fragmentShaderID);

	glDeleteShader(vertexShaderID);
	glDeleteShader(fragmentShaderID);

	return programID;
}

GLuint mat2texture(const cv::Mat& img) {
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Give the image to OpenGL
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGB,
		img.cols,
		img.rows,
		0,
		GL_RGB,
		GL_UNSIGNED_BYTE,
		img.data);

	// Set texture interpolation methods for minification and magnification
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);

	return textureID;
}

void drawBackground(const cv::Mat& img, const GLuint& programID) {
	GLuint backgroundVertexArrayID;
	glGenVertexArrays(1, &backgroundVertexArrayID);
	glBindVertexArray(backgroundVertexArrayID);

	const GLfloat backgroundVertexBufferData[] = {
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,

		1.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 0.0f
	};
	const GLfloat backgroundUVBufferData[] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,

		1.0f, 1.0f,
		0.0f, 1.0f,
		1.0f, 0.0f
	};

	GLuint backgroundVertexBuffer;
	glGenBuffers(1, &backgroundVertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, backgroundVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(backgroundVertexBufferData),
		backgroundVertexBufferData, GL_STATIC_DRAW);

	GLuint backgroundUVBuffer;
	glGenBuffers(1, &backgroundUVBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, backgroundUVBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(backgroundUVBufferData),
		backgroundUVBufferData, GL_STATIC_DRAW);

	// Use the shaders
	glUseProgram(programID);

	GLuint texture = mat2texture(img);
	GLuint textureID = glGetUniformLocation(programID,
		"myTextureSampler");

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(textureID, 0);

	// 1st attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, backgroundVertexBuffer);
	glVertexAttribPointer(
		0,
		3,
		GL_FLOAT,
		GL_FALSE,
		0,
		(void*)0
	);

	// 2nd attribute buffer : UVs
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, backgroundUVBuffer);
	glVertexAttribPointer(
		1,
		2,
		GL_FLOAT,
		GL_FALSE,
		0,
		(void*)0
	);

	// Draw the triangle !
	glDrawArrays(GL_TRIANGLES, 0, 2 * 3);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	glDeleteTextures(1, &texture);
}

void drawCube(
	const GLuint &objID,
	const GLuint &glMVPMatID,
	const glm::mat4& glMVPMat,
	const GLuint& vertexBuffer,
	const GLuint& colorBuffer
)
{
	// Use our shader
	glUseProgram(objID);

	// Send our transformation to the currently bound shader, 
	// in the "glMVPMat" uniform
	glUniformMatrix4fv(glMVPMatID, 1, GL_FALSE, &glMVPMat[0][0]);

	// 1rst attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glVertexAttribPointer(
		0,  // attribute. No particular reason for 0, but must match the layout in the shader.
		3,  // size
		GL_FLOAT,  // type
		GL_FALSE,  // normalized?
		0,  // stride
		(void*)0  // array buffer offset
	);

	// 2nd attribute buffer : colors
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, colorBuffer);
	glVertexAttribPointer(
		1,  // attribute. No particular reason for 1, but must match the layout in the shader.
		3,  // size
		GL_FLOAT,  // type
		GL_FALSE,  // normalized?
		0,  // stride
		(void*)0  // array buffer offset
	);

	// Draw the triangle !
	glDrawArrays(GL_TRIANGLES, 0, 12 * 3); // 12*3 indices starting at 0 -> 12 triangles

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}

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
	cv::Ptr<cv::aruco::DetectorParameters> parameters = cv::aruco::DetectorParameters::create();
	cv::aruco::detectMarkers(img, markerDict, corners, ids, parameters, cv::noArray(), camMat, distCoeffs);
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
