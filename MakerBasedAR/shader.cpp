#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
using namespace std;

#include <stdlib.h>
#include <string.h>
#include <opencv2/opencv.hpp>
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.hpp"

bool initializeGLAndMakeWindow(GLFWwindow* &window, int width, int height, char* windowName)
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

GLuint loadShaders(const char * vertexFilePath, const char * fragmentFilePath) {
	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertexFilePath, std::ios::in);
	if (VertexShaderStream.is_open()) {
		std::stringstream sstr;
		sstr << VertexShaderStream.rdbuf();
		VertexShaderCode = sstr.str();
		VertexShaderStream.close();
	}
	else {
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertexFilePath);
		getchar();
		return 0;
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragmentFilePath, std::ios::in);
	if (FragmentShaderStream.is_open()) {
		std::stringstream sstr;
		sstr << FragmentShaderStream.rdbuf();
		FragmentShaderCode = sstr.str();
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertexFilePath);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragmentFilePath);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	// Link the program
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}


	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

GLuint mat2texture(const cv::Mat& input_image) {
	GLuint texture_id;
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);

	// Give the image to OpenGL
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RGB,
		input_image.cols,
		input_image.rows,
		0,
		GL_RGB,
		GL_UNSIGNED_BYTE,
		input_image.data);

	// Set texture interpolation methods for minification and magnification
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);

	return texture_id;
}

// Load obj file
// If succeed, give all the vertices and normals, and return true
// If fail, return false
bool loadObj(
	const std::string& input_filename,
	std::vector<glm::vec3>& output_vertices,
	std::vector<glm::vec3>& output_normals) {
	// Use a stream to read ply file
	std::ifstream read_file(input_filename, std::ios::in);

	// If faill to open the file, return false
	if (!read_file.is_open()) {
		return false;
	}

	// If the output vector is not empty, clear it
	if (!output_vertices.empty()) {
		output_vertices.clear();
	}
	if (!output_normals.empty()) {
		output_normals.clear();
	}

	// Used to record the header in ply file
	std::string header;
	// Used to recorder the number of vertex and the number of face
	size_t num_of_vertex = 0;
	size_t num_of_face = 0;
	// Store the coordinates of normal
	std::vector<glm::vec3> normal_coordinates;
	// Store the coordinates of vertex
	std::vector<glm::vec3> vertex_coordinates;
	// The index indicates the normal that a face has
	std::vector<int> face_normal_index;
	// The index indicates the vertex that a face has
	std::vector<int> face_vertex_index;

	while (!read_file.eof()) {
		// Read the first header of a line
		read_file >> header;

		// If the header indicates "#", skip
		if (header == "#") {
			// Skip and go to next line
			std::string line;
			std::getline(read_file, line);
			continue;
		}

		// If the header indicates "vn", it means normal
		if (header == "vn") {
			float normal_x, normal_y, normal_z;
			// x-coordinate
			read_file >> normal_x;
			// y-coordinate
			read_file >> normal_y;
			// z-coordinate
			read_file >> normal_z;

			normal_coordinates.push_back(glm::vec3(
				normal_x, normal_y, normal_z));

			// Skip the rest and go to next line
			std::string line;
			std::getline(read_file, line);
			continue;
		}

		// If the header indicates "v", it means vertex
		if (header == "v") {
			float vertex_x, vertex_y, vertex_z;
			// x-coordinate
			read_file >> vertex_x;
			// y-coordinate
			read_file >> vertex_y;
			// z-coordinate
			read_file >> vertex_z;

			vertex_coordinates.push_back(glm::vec3(
				vertex_x, vertex_y, vertex_z));

			// Skip the rest and go to next line
			std::string line;
			std::getline(read_file, line);
			continue;
		}

		// If the header indicates "f", it means face index
		if (header == "f") {
			int face_normal_x, face_normal_y, face_normal_z;
			int face_vertex_x, face_vertex_y, face_vertex_z;
			// Used to skip "//"
			char character;

			read_file >> face_normal_x;
			face_normal_index.push_back(face_normal_x);
			read_file >> character >> character;
			read_file >> face_vertex_x;
			face_vertex_index.push_back(face_vertex_x);

			read_file >> face_normal_y;
			face_normal_index.push_back(face_normal_y);
			read_file >> character >> character;
			read_file >> face_vertex_y;
			face_vertex_index.push_back(face_vertex_y);

			read_file >> face_normal_z;
			face_normal_index.push_back(face_normal_z);
			read_file >> character >> character;
			read_file >> face_vertex_z;
			face_vertex_index.push_back(face_vertex_z);

			// Skip the rest and go to next line
			std::string line;
			std::getline(read_file, line);
			continue;
		}
	}

	for (size_t i = 0; i < face_normal_index.size(); i++) {
		size_t current_index = face_normal_index[i] - 1;
		output_normals.push_back(normal_coordinates[current_index]);
	}

	for (size_t i = 0; i < face_vertex_index.size(); i++) {
		size_t current_index = face_vertex_index[i] - 1;
		output_vertices.push_back(vertex_coordinates[current_index]);
	}

	if (!output_normals.empty() && !output_vertices.empty()) {
		read_file.close();
		return true;
	}

	read_file.close();
	return false;
}

// Draw the frame (background) captured by my PC's internal camera
void drawBackground(const cv::Mat& input_image, const GLuint& programID) {
	GLuint background_vertex_array_id;
	glGenVertexArrays(1, &background_vertex_array_id);
	glBindVertexArray(background_vertex_array_id);

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

	GLuint texture = mat2texture(input_image);
	GLuint texture_id = glGetUniformLocation(programID,
		"myTextureSampler");

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(texture_id, 0);

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

void drawShadingObj(
	const std::vector<glm::vec3>& vertices,
	const std::vector<glm::vec3>& normals,
	const glm::mat4& glModelMat,
	const glm::mat4& glViewMat,
	const glm::mat4& glProjMat,
	const GLuint& programID
) 
{
	GLuint vertexArrayID;
	glGenVertexArrays(1, &vertexArrayID);
	glBindVertexArray(vertexArrayID);

	GLuint glMVPMatID = glGetUniformLocation(programID, "MVP");
	GLuint glViewMatID = glGetUniformLocation(programID, "V");
	GLuint glModelMatID = glGetUniformLocation(programID, "M");

	GLuint vertexBuffer;
	glGenBuffers(1, &vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3),
		&vertices[0], GL_STATIC_DRAW);

	GLuint normalBuffer;
	glGenBuffers(1, &normalBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3),
		&normals[0], GL_STATIC_DRAW);

	glUseProgram(programID);
	GLuint light_id =
		glGetUniformLocation(programID, "LightPosition_worldspace");

	glUseProgram(programID);
	glm::mat4 mvp = glProjMat * glViewMat * glModelMat;
	glUniformMatrix4fv(glMVPMatID, 1, GL_FALSE, &mvp[0][0]);
	glUniformMatrix4fv(glModelMatID, 1, GL_FALSE, &glModelMat[0][0]);
	glUniformMatrix4fv(glViewMatID, 1, GL_FALSE, &glViewMat[0][0]);

	glm::vec3 lightPos = glm::vec3(-3, -4, 1);
	glUniform3f(light_id, lightPos.x, lightPos.y, lightPos.z);

	// 1st attribute buffer : vertices
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
	glVertexAttribPointer(
		0,
		3,
		GL_FLOAT,
		GL_FALSE,
		0,
		(void*)0
	);

	// 2nd attribute buffer : colors
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
	glVertexAttribPointer(
		1,
		3,
		GL_FLOAT,
		GL_FALSE,
		0,
		(void*)0
	);

	// Draw the triangle
	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}