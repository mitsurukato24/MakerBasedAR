#pragma once

GLuint loadShaders(const char * vertexFilePath, const char * fragmentFilePath);

bool loadObj(
	const std::string& input_filename,
	std::vector<glm::vec3>& output_vertices,
	std::vector<glm::vec3>& output_normals);

bool initializeGLAndMakeWindow(GLFWwindow* &window, int width, int height, char* windowName);

// Draw the frame (background) captured by my PC's internal camera
void drawBackground(const cv::Mat& input_image, const GLuint& program_id);

GLuint mat2texture(const cv::Mat& input_image);

void drawShadingObj(
	const std::vector<glm::vec3>& vertices,
	const std::vector<glm::vec3>& normals,
	const glm::mat4& glModelMat,
	const glm::mat4& glViewMat,
	const glm::mat4& glProjMat,
	const GLuint& programID
);