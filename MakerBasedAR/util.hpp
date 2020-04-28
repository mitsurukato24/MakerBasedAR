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
);