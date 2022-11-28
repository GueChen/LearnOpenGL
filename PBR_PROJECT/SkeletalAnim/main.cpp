#ifdef WIN32
#include <Windows.h>
#endif
#include <glad/glad.h>

#include "camera.h"
#include "custom_glfw_window.h"

#include <glm/glm.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include "animator.h"
#include "animation.h"


#define VERT_PATH(name) SHADER_PATH_PREFIX#name".vert"
#define FRAG_PATH(name) SHADER_PATH_PREFIX#name".frag"

using namespace glm;
GLFWCustomWindow window("orge dancing", scr_width, scr_height);
Camera			 camera(vec3(0.0f, 0.0f, 5.0f));
Model			 model(MODEL_PATH_DIR"/vampire/dancing_vampire.dae");
Animation		 anim(MODEL_PATH_DIR"/vampire/dancing_vampire.dae", &model);
Animator		 animator(&anim);
bool			 g_cursor_entered = false;
float			 g_anim_speed = 1.0f;
void InitWindowSetting();
void InitGUI();
void main_loop();
void ProcessInput(GLFWwindow* window, float delta_time);

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow) {
	
	InitGUI();
	InitWindowSetting();
	
	window.m_loop_func = main_loop;	

	window.Run();

}

void RenderScene();
void RenderGUI();

void main_loop() {

	static float last_frame = static_cast<float>(glfwGetTime());
	float  current_frame = static_cast<float>(glfwGetTime());
	float  delta_time = current_frame - last_frame;
	last_frame = current_frame;

	ProcessInput(window.m_window_ptr, delta_time);
	animator.UpdateAnimation(g_anim_speed * delta_time);

	RenderScene();

	RenderGUI();
}

void RenderScene()
{
	static Shader anim_shader(VERT_PATH(skelanim), FRAG_PATH(mesh_render));
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	anim_shader.use();

	glm::mat4 proj = glm::perspective(glm::radians(camera.Zoom), (float)scr_width / std::max((float)scr_height, 1.0f), 0.1f, 100.0f);
	glm::mat4 view = camera.GetViewMatrix();

	anim_shader.setMat4("proj", proj);
	anim_shader.setMat4("view", view);

	auto& transforms = animator.GetBoneMatrices();

	for (int i = 0; i < transforms.size(); ++i) {
		anim_shader.setMat4("bone_matrices_arr[" + std::to_string(i) + "]", transforms[i]);
	}

	glm::mat4 model_mat = glm::mat4(1.0f);
	model_mat = glm::translate(model_mat, glm::vec3(0.2f, -1.0f, 0.0f));
	model_mat = glm::scale(model_mat, glm::vec3(1.5f));
	anim_shader.setMat4("model", model_mat);
	model.Draw(anim_shader);
}

void RenderGUI() {
	if (g_cursor_entered) {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();			
		ImGui::StyleColorsLight();
		ImGui::NewFrame();		
		ImGui::Begin("status bar", 0,
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoNav);
		ImGui::Button(" ", ImVec2(450, 5)); 
		ImGui::SameLine();
		if (ImGui::Button("x", ImVec2(20, 20))) {
			glfwSetWindowShouldClose(window.m_window_ptr, true);
		}
		ImGui::End();
		ImGui::StyleColorsClassic();
		ImGui::Begin("  ", 0,
			ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoNav);
		ImGui::Text("speed"); ImGui::SameLine();
		ImGui::SliderFloat("   ", &g_anim_speed, 0.05f, 5.0f);
		ImGui::End();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}
}

void CursorEnterCallback(GLFWwindow* window, int entered) {
	g_cursor_entered = entered;
}

void InitWindowSetting()
{
	int count;
	GLFWmonitor** monitors = glfwGetMonitors(&count);
	const GLFWvidmode* video_mode = glfwGetVideoMode(monitors[0]);
	int monitor_x, monitor_y;
	glfwGetMonitorPos(monitors[0], &monitor_x, &monitor_y);	
	glfwSetWindowPos(window.m_window_ptr, video_mode->width *( 4.0 /5.0), video_mode->height * (4.0 / 5.0));
	glfwSetWindowAttrib(window.m_window_ptr, GLFW_DECORATED, GLFW_FALSE);
	glfwSetWindowAttrib(window.m_window_ptr, GLFW_FLOATING, GLFW_TRUE);
	glfwSetWindowSize(window.m_window_ptr, video_mode->width / 5, video_mode->height / 5);

	glfwSetCursorEnterCallback(window.m_window_ptr, CursorEnterCallback);	
}

void InitGUI()
{
	ImGui::CreateContext();	
	ImGui::StyleColorsDark();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	
	ImGui_ImplGlfw_InitForOpenGL(window.m_window_ptr, true);
	ImGui_ImplOpenGL3_Init("#version 450");
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

