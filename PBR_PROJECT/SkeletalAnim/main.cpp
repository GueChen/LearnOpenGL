#include <glad/glad.h>

#include "camera.h"
#include "custom_glfw_window.h"

#include <glm/glm.hpp>

#include <iostream>

using namespace glm;
GLFWCustomWindow window("good window", scr_width, scr_height);

Camera camera(vec3(0.0f, 0.0f, 3.0f));

void main_loop();
void ProcessInput(GLFWwindow* window, float delta_time);

int main() {
	
	window.m_loop_func = main_loop;	
	
	window.Run();

}

void main_loop(){
	static float last_frame = static_cast<float>(glfwGetTime());
	float  current_frame = static_cast<float>(glfwGetTime());
	float  delta_time = current_frame - last_frame;
	last_frame = current_frame;

	ProcessInput(window.m_window_ptr, delta_time);
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