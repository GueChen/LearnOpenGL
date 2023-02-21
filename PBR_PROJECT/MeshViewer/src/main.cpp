#ifdef _WIN32
#include <Windows.h>
#endif

#include <glad/glad.h>

// pbr_proj local file
#include "custom_glfw_window.h"

// common lib
#include "shader.h"
#include "camera.h"
#include "model.h"
#include "logger.h"
#define STB_IMAGE_IMPLEMENTATION

#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <filesystem>
#include <iostream>
#include <cstdint>
#include <thread>
#include <format>
#include <string>
#include <vector>

#define VERT_PATH(name) SHADER_PATH_PREFIX#name".vert"
#define FRAG_PATH(name) SHADER_PATH_PREFIX#name".frag"

void ProcessInput  (GLFWwindow* w, float delta_time);
vector<string> Split(const string& str, char symbol);


#ifdef _WIN32
HANDLE CreateNamedPipeInWindows();
void   ServerTask(HANDLE pipe);
#endif

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

int main() {
	
	float		last_frame  = 0.0f;
	GLFWwindow* window		= nullptr;
	thread      read_thread;
#ifdef _WIN32
	HANDLE		pipe = INVALID_HANDLE_VALUE;

	Logger::Message("create pipe resource...");
	pipe = CreateNamedPipeInWindows();
	if (pipe == INVALID_HANDLE_VALUE) {
		Logger::Error("create pipe failed");
		goto finish;
	}
	Logger::Message("Pipe Initialize Successed");
	read_thread = thread(ServerTask, pipe);
#endif

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	window = glfwCreateWindow(scr_width, scr_height, "MeshViewer", NULL, NULL);
	
	
	if (!window) {
		Logger::Error("Create Window Failed");		
		goto finish;
	}
	Logger::Message("Window Initialized!");

	glfwMakeContextCurrent(window);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		Logger::Error("Glad Initialized Failed");
		goto finish;
	}

	while (!glfwWindowShouldClose(window)) {
		float current_frame = static_cast<float>(glfwGetTime());
		float delta_time    = current_frame - last_frame;
		last_frame = current_frame;

		//ProcessInput(window, delta_time);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glfwPollEvents();
		glfwSwapBuffers(window);
	}

finish:
	glfwTerminate();
	Logger::Message("exist");
	return 0;
}

#ifdef _WIN32
HANDLE CreateNamedPipeInWindows()
{
	HANDLE h_pipe    = INVALID_HANDLE_VALUE;
	BOOL   connected = FALSE;
	h_pipe = CreateNamedPipe(
		"\\\\.\\pipe\\mesh_viewer_pipe",
		PIPE_ACCESS_INBOUND,
		PIPE_WAIT,
		PIPE_UNLIMITED_INSTANCES,
		512,
		512,
		0,
		NULL
	);

	if (h_pipe == INVALID_HANDLE_VALUE) {
		Logger::Error("pipe created error!");		
		goto finish;
	}

	connected = ConnectNamedPipe(h_pipe, NULL) ? 
		TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
	if (!connected) {
		CloseHandle(h_pipe);
		h_pipe = INVALID_HANDLE_VALUE;
	}
finish:
	return h_pipe;
}

void ServerTask(HANDLE pipe)
{	
	static char vertex_symbol[] = "vertex:";
	static char face_symbol[]   = "face:";
	string buffer(1024, '\0');
	DWORD receive_bytes = 0;
	int vertex_nb = 0, face_nb = 0;
	while (ReadFile(pipe, buffer.data(), 1024, &receive_bytes, NULL)) {
		vector<string> datas = Split(buffer, '\n');
		for (auto& rawdata : datas) {
			if (strncmp(rawdata.data(), vertex_symbol, strlen(vertex_symbol)) == 0) {				
				vector<string> raw_vals = Split(rawdata.substr(strlen(vertex_symbol)), ',');
				vector<double> vals; vals.reserve(raw_vals.size());
				for (auto& s : raw_vals) {
					vals.push_back(stod(s));
				}
				++vertex_nb;
#ifdef _DISPLAY_RECEIVE_VERTEX
				Logger::Message(format("vertex -- ({:>6},{:>6},{:>6})\n", vals[0], vals[1], vals[2]));
#endif
			}
			else if (strncmp(rawdata.data(), face_symbol, strlen(face_symbol)) == 0) {
				vector<string> raw_vals = Split(rawdata.substr(strlen(face_symbol)), ',');
				vector<int> vals; vals.reserve(raw_vals.size());
				for (auto& s : raw_vals) {
					vals.push_back(stoi(s));
				}				
				++face_nb;
#ifdef _DISPLAY_RECEIVE_INDICES
				Logger::Message(format("face index -- {:>2} {:>2} {:>2}\n", vals[0], vals[1], vals[2]));
#endif
			}			
		}
		buffer.clear();
		buffer.resize(1024);
		Logger::Message(format("Static -- vertex count - {}, face count - {}", vertex_nb, face_nb));
#ifdef _DISPLAY_PIPE_MSG
		Logger::Message(format("receive -- {:}", buffer));
#endif
	}
	Logger::Message(format("Static -- vertex count - {}, face count - {}", vertex_nb, face_nb));
	Logger::Warning("server quit");
}
#endif

vector<string> Split(const string& str, char symbol)
{
	vector<string> ret;
	string temp;
	for (auto& c : str) {
		if (c == symbol) {
			if (!temp.empty()) {
				ret.push_back(temp);
			}
			temp.clear();
		}
		else {
			temp.push_back(c);
		}
	}
	if (!temp.empty()) ret.push_back(temp);
	return ret;
}
