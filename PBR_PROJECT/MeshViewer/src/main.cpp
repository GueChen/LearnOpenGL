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
#include "mesh.h"
#include "logger.h"
#define STB_IMAGE_IMPLEMENTATION

#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <filesystem>
#include <iostream>
#include <numeric>
#include <algorithm>
#include <cstdint>
#include <thread>
#include <mutex>
#include <format>
#include <string>
#include <vector>
#include <map>

//#define _DISPLAY_RECEIVE_FACET

#define VERT_PATH(name) SHADER_PATH_PREFIX#name".vert"
#define FRAG_PATH(name) SHADER_PATH_PREFIX#name".frag"

void RenderScene();
void RenderGUI	();

void ProcessInput     (GLFWwindow* w, float delta_time);
void MouseMoveCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback   (GLFWwindow* window, double xoffset, double yoffset);

vector<string> Split  (const string& str, char symbol);

void InitializeImGUI  (GLFWwindow* window);

#ifdef _WIN32
HANDLE CreateNamedPipeInWindows();
void   ServerTask(HANDLE pipe);
#endif

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

vector<Vertex>       vertices;
vector<unsigned int> indices;
mutex  mut;

int main() {
	
	float		last_frame  = 0.0f;
	GLFWwindow* window		= nullptr;
	thread      read_thread;
	
#ifdef _WIN32
	HANDLE		pipe = INVALID_HANDLE_VALUE;
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
	glfwHideWindow(window);
	glfwMakeContextCurrent(window);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		Logger::Error("Glad Initialized Failed");
		goto finish;
	}
	
	glfwSetCursorPosCallback(window, MouseMoveCallback);
	glfwSetScrollCallback(window, ScrollCallback);

	InitializeImGUI(window);
#ifdef _WIN32
	Logger::Message("create pipe resource...");
	pipe = CreateNamedPipeInWindows();
	if (pipe == INVALID_HANDLE_VALUE) {
		Logger::Error("create pipe failed");
		goto finish;
	}
	Logger::Message("Pipe Initialize Successed");
	read_thread = thread(ServerTask, pipe);
#endif
	glfwShowWindow(window);

	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	while (!glfwWindowShouldClose(window)) {
		float current_frame = static_cast<float>(glfwGetTime());
		float delta_time    = current_frame - last_frame;
		last_frame = current_frame;

		ProcessInput(window, delta_time);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		RenderScene();
		
		glfwPollEvents();
		glfwSwapBuffers(window);
	}

finish:
	glfwTerminate();
#ifdef _WIN32
	read_thread.join();
#endif
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

void RenderScene(){
	static Shader half_alpha_shader(VERT_PATH(simple_vert), FRAG_PATH(tile_color));
	static Shader draw_line_shader (VERT_PATH(simple_vert), FRAG_PATH(draw_line));
	static Mesh*  mesh = nullptr;
		
	if (!mesh) {
		lock_guard<mutex> lock(mut);
		if (!vertices.empty() && !indices.empty()) {
			mesh = new Mesh(vertices, indices, {});
		}
	}
	else {
		glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), (float)scr_width / scr_height, 0.1f, 100.0f);
		glm::mat4 view = camera.GetViewMatrix();

		//glEnable(GL_BLEND);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		half_alpha_shader.use();
		// global settings
		half_alpha_shader.setMat4("model", glm::mat4(1.0f));
		half_alpha_shader.setMat4("view", view);
		half_alpha_shader.setMat4("proj", proj);
		half_alpha_shader.setVec3("camera_pos", camera.pos);
		// material settings
		half_alpha_shader.setInt  ("pri_tot_num", indices.size() / 3);
		half_alpha_shader.setVec4 ("color_start", glm::vec4(0.85f, 0.2f, 0.95f, 0.6f));
		half_alpha_shader.setVec4 ("color_end", glm::vec4(0.95f, 0.55f, 0.55f, 0.6f));
		half_alpha_shader.setFloat("roughness", 0.05f);
		half_alpha_shader.setFloat("metalic", 0.05f);
		half_alpha_shader.setFloat("ao", 1.0f);
		// light settings
		half_alpha_shader.setVec3("light_dir",   glm::vec3(1.0f, 1.0f, 1.0f));
		half_alpha_shader.setVec3("light_color", glm::vec3(1.0f));

		glCullFace(GL_FRONT);
		half_alpha_shader.setFloat("alpha", 0.3f);
		mesh->Draw(half_alpha_shader);
		
		glCullFace(GL_BACK);
		half_alpha_shader.setFloat("alpha", 0.7f);		
		mesh->Draw(half_alpha_shader);

		glDisable(GL_BLEND);
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glLineWidth(2.0f);
		draw_line_shader.use();
		draw_line_shader.setMat4("model", glm::mat4(1.0f));
		draw_line_shader.setMat4("view", view);
		draw_line_shader.setMat4("proj", proj);
		
		mesh->Draw(draw_line_shader);
	}
}

void RenderGUI() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::Begin("Setting Box");


	ImGui::End();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ServerTask(HANDLE pipe)
{	
	static char vertex_symbol[] = "vertex:";
	static char face_symbol[]   = "face:";
	string buffer(1024, '\0');
	DWORD receive_bytes = 0;
	int vertex_nb = 0, face_nb = 0;
		
	map<unsigned int, unsigned int> vert_map; // map id to index	
	map<unsigned int, unsigned int> idx_map;

	while (ReadFile(pipe, buffer.data(), 1024, &receive_bytes, NULL)) {
		vector<string> datas = Split(buffer, '\n');
		int ori_vertex_nb = vertex_nb, ori_face_nb = face_nb;
		lock_guard<mutex> lock(mut);
		for (auto& rawdata : datas) {
			if (strncmp(rawdata.data(), vertex_symbol, strlen(vertex_symbol)) == 0) {				
				vector<string> raw_vals  = Split(rawdata.substr(strlen(vertex_symbol)), ',');
				unsigned int   vertex_id = stoi(raw_vals.front());								
				
				Vertex vertex;				
				vertex.pos = glm::vec3(stod(raw_vals[1]), stod(raw_vals[2]), stod(raw_vals[3]));
				vertex.norm = glm::zero<glm::vec3>();
				vertices.push_back(vertex);

				vert_map[vertex_nb] = vertex_id;
				idx_map[vertex_id]  = vertex_nb;
				++vertex_nb;
#ifdef _DISPLAY_RECEIVE_VERTEX
				vector<double> vals; vals.reserve(raw_vals.size() - 1);
				for (auto it = raw_vals.begin() + 1; it < raw_vals.end(); ++it) {
					vals.push_back(stod(*it));
				}
				Logger::Message(format("vertex -- ({:>6},{:>6},{:>6})\n", vals[0], vals[1], vals[2]));
#endif
			}
			else if (strncmp(rawdata.data(), face_symbol, strlen(face_symbol)) == 0) {
				vector<string> raw_vals = Split(rawdata.substr(strlen(face_symbol)), ',');
				for (int i = 0; i < 3; ++i) {
					indices.push_back(idx_map[stoi(raw_vals[i])]);
				}
				glm::vec3 norm = glm::vec3(stod(raw_vals[3]), stod(raw_vals[4]), stod(raw_vals[5]));
				vertices[indices[face_nb * 3 + 0]].norm += norm;
				vertices[indices[face_nb * 3 + 1]].norm += norm;
				vertices[indices[face_nb * 3 + 2]].norm += norm;
				++face_nb;
#ifdef _DISPLAY_RECEIVE_FACET
				vector<int>    vals;  vals.reserve(3);
				vector<double> norms; norms.reserve(3);
				for (int i = 0; i < raw_vals.size(); ++i) {
					if (i < 3) {
						vals.push_back(stoi(raw_vals[i]));
					}
					else {
						norms.push_back(stod(raw_vals[i]));
					}
				}
				Logger::Message(format("face index -- {:>2} {:>2} {:>2}", vals[0], vals[1], vals[2]));
				Logger::Message(format("face normal-- ({:},{:},{:})\n", norms[0], norms[1], norms[2]));
#endif
			}			
		}

		buffer.clear();
		buffer.resize(1024);
#ifdef _DISPLAY_GEOMETRY_STATIC
		Logger::Message(format("Static -- vertex count - {}, face count - {}", vertex_nb, face_nb));
#endif
#ifdef _DISPLAY_PIPE_RECEIVE_MSG
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

void ProcessInput(GLFWwindow* window, float delta_time)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS) {
		rotate_flag = true;
	}
	else {
		rotate_flag = false;
	}
}

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void MouseMoveCallback(GLFWwindow* window, double xpos, double ypos)
{
	static float  last_x = 800.0f / 2.0f;
	static float  last_y = 600.0f / 2.0f;
	static bool   first_mouse = true;

	if (first_mouse) {
		last_x = xpos;
		last_y = ypos;
		first_mouse = false;
	}

	float xoffset = xpos - last_x;
	float yoffset = ypos - last_y;

	last_x = static_cast<float>(xpos);
	last_y = static_cast<float>(ypos);

	if (rotate_flag) {
		camera.ProcessPerspectiveView(xoffset, -yoffset);
	}
}

void InitializeImGUI(GLFWwindow* window)
{
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 450");
}