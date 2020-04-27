#pragma once

bool initializeGLAndMakeWindow(GLFWwindow* &window, const int width, const int height, const char* windowName);

bool getCameraParamsFromYML(const std::string filename, int &imgWidth, int &imgHeight, int &focusParam, cv::Mat &camMat, cv::Mat &distCoeffs);

GLuint loadShaders(const char* vertexFilePath, const char* fragmentFilePath);

GLuint mat2texture(const cv::Mat& img);

void drawBackground(const cv::Mat& img, const GLuint& programID);

void drawCube(
	const GLuint &objID,
	const GLuint &glMVPMatID,
	const glm::mat4& glMVPMat,
	const GLuint& vertexBuffer,
	const GLuint& colorBuffer
);