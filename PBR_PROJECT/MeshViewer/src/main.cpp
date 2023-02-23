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

void ProcessInput       (GLFWwindow* w, float delta_time);
void MouseMoveCallback  (GLFWwindow* window, double xpos, double ypos);
void ScrollCallback     (GLFWwindow* window, double xoffset, double yoffset);
void FramebufferCallback(GLFWwindow* window, int width, int height);

void GenFrameBuffer(int width, int height);
void CleanFrameBuffer();

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

unsigned int texture = 0, fbo = 0, rbo = 0;
unsigned int render_width = scr_width, render_height = scr_height;

glm::vec3 color_start = glm::vec3(0.4f, 0.1f, 0.9f);
glm::vec3 color_end   = glm::vec3(0.6f, 0.6f, 0.0f);

float	  roughness   = 0.05f;
float     metallic    = 0.05f;

struct DirectLight {
	glm::vec3 dir;
	glm::vec3 color;
	DirectLight(glm::vec3 _dir, glm::vec3 _col) : dir(_dir), color(_col) {};
};

DirectLight glb_light = DirectLight(glm::vec3(-1.0f), glm::vec3(1.0f));

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
	glfwSetFramebufferSizeCallback(window, FramebufferCallback);

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
	GenFrameBuffer(render_width, render_height);

	
	while (!glfwWindowShouldClose(window)) {
		float current_frame = static_cast<float>(glfwGetTime());
		float delta_time    = current_frame - last_frame;
		last_frame = current_frame;

		ProcessInput(window, delta_time);
		glfwMakeContextCurrent(window);
		
		RenderScene();
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		RenderGUI();

		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
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
	
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);	
	glViewport(0, 0, render_width, render_height);
	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (!mesh) {
		lock_guard<mutex> lock(mut);
		if (!vertices.empty() && !indices.empty()) {
			mesh = new Mesh(vertices, indices, {});
		}
	}
	else {
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), (float)render_width / render_height, 0.1f, 100.0f);
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
		half_alpha_shader.setVec3 ("color_start", color_start);
		half_alpha_shader.setVec3 ("color_end",   color_end);
		half_alpha_shader.setFloat("roughness",   roughness);
		half_alpha_shader.setFloat("metalic",     metallic);
		half_alpha_shader.setFloat("ao", 1.0f);
		// light settings
		half_alpha_shader.setInt ("direct_light_num", 1);
		half_alpha_shader.setVec3("light_dir[0]",   glb_light.dir);
		half_alpha_shader.setVec3("light_color[0]", glb_light.color);

		glCullFace(GL_FRONT);
		half_alpha_shader.setFloat("alpha", 0.3f);
		mesh->Draw(half_alpha_shader);
		
		glCullFace(GL_BACK);
		half_alpha_shader.setFloat("alpha", 0.7f);		
		mesh->Draw(half_alpha_shader);

		glDisable(GL_BLEND);
		
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		//glEnable(GL_LINE_STIPPLE);
		draw_line_shader.use();
		draw_line_shader.setMat4("model", glm::mat4(1.0f));
		draw_line_shader.setMat4("view", view);
		draw_line_shader.setMat4("proj", proj);
		
		glLineWidth(1.0f);
		glDepthFunc(GL_GREATER);
		draw_line_shader.setVec3("color", glm::vec3(0.55f));
		mesh->Draw(draw_line_shader);

		glLineWidth(2.0f);
		glDepthFunc(GL_LESS);
		draw_line_shader.setVec3("color", glm::vec3(1.0f));
		mesh->Draw(draw_line_shader);
		glEnable(GL_DEPTH_TEST);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderGUI() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	
	ImGui::NewFrame();
	////ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
	////ImGui::SetNextWindowPos(ImVec2(0, 0));
	static bool open = true;
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
		window_flags |= ImGuiWindowFlags_NoBackground;
	ImGui::Begin("DockSpace Demo", &open, window_flags);

	ImGui::PopStyleVar(2);
	
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
	{
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	}
	ImGui::End();
	
	ImGui::Begin("Setting");
	if (ImGui::CollapsingHeader("material")) {
		ImGui::SliderFloat("roughness", &roughness, 0.04f, 1.0f, "%.2f");
		ImGui::SliderFloat("metallic", &metallic, 0.0f, 1.0f, "%.2f");
		ImGui::ColorEdit3("start", glm::value_ptr(color_start));
		ImGui::ColorEdit3("end",   glm::value_ptr(color_end));
	}
	
	if (ImGui::CollapsingHeader("lights")) {
		if (ImGui::SliderFloat3("dir", glm::value_ptr(glb_light.dir), -1.0f, 1.0f, "%.2f")) {
			glb_light.dir =  glm::normalize(glb_light.dir);
		}
		ImGui::ColorEdit3("color", glm::value_ptr(glb_light.color));
	}
	ImGui::Text("No Implementation");
	ImGui::End();

	ImGui::Begin("Viewport");
	ImVec2 viewport_panelsize = ImGui::GetContentRegionAvail();
		
	ImGui::Image((void*)texture, ImVec2{ viewport_panelsize.x, viewport_panelsize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
	
	ImGui::End();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	if (render_width != viewport_panelsize.x || render_height != viewport_panelsize.y) {
		render_width = viewport_panelsize.x;
		render_height = viewport_panelsize.y;
		CleanFrameBuffer();
		GenFrameBuffer(render_width, render_height);
	}
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
void FramebufferCallback(GLFWwindow* window, int width, int height) {
	CleanFrameBuffer();
	GenFrameBuffer(render_width, render_height);
	
}

void GenFrameBuffer(int width, int height) {
	// intialize fbo
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// initialize rbo
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	// initialize texture
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
}

void CleanFrameBuffer() {
	if (fbo) {
		glDeleteFramebuffers(1, &fbo);
		glDeleteRenderbuffers(1, &rbo);
		glDeleteTextures(1, &texture);
	}
}

void InitializeImGUI(GLFWwindow* window)
{
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 450");
}