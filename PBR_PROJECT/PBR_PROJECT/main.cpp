#include "shader.h"
#include "camera.h"
#include "model.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <iostream>
#include <cstdint>

using namespace std;
using namespace glm;

/*____________________________________const varaiable____________________________________*/
const uint32_t kScrWidth  = 1280;
const uint32_t kScrHeight = 720;

/*____________________________________function declarations_______________________________*/
GLFWwindow* InitializeWindow();
void		ProcessInput	(GLFWwindow* window, float delta_time);
void		RenderPass		();

/*____________________________________global varaiable____________________________________*/ 
Camera camera(vec3(0.0f, 0.0f, 3.0f));

int main()
{	
	GLFWwindow* window	   = InitializeWindow();
	float       last_frame = 0.0f;

	if (window == nullptr) goto program_end;

	while (!glfwWindowShouldClose(window)) {
		float  current_frame = static_cast<float>(glfwGetTime());
		float  delta_time = current_frame - last_frame;
		last_frame = current_frame;

		ProcessInput(window, delta_time);

		RenderPass  ();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

program_end:
	glfwTerminate();
	return 0;
}

void RenderPass()
{
	assert(false && "No Implementation");
}

void FrameSizeChangeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
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

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void ProcessInput(GLFWwindow* window, float delta_time)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD,	 delta_time);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, delta_time);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT,	 delta_time);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT,	 delta_time);
}

GLFWwindow* InitializeWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(kScrWidth, kScrHeight, "PBR_EXAMPLE", NULL, NULL);
	glfwMakeContextCurrent(window);
	if (window == NULL) {
		cout << "Create Window Error\n";
		glfwTerminate();
		return nullptr;
	}

	glfwSetFramebufferSizeCallback(window, FrameSizeChangeCallback);
	glfwSetCursorPosCallback(window, MouseMoveCallback);
	glfwSetScrollCallback(window, ScrollCallback);

	// no cursor display
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// loading glad
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		cout << "GLAD Initialize ERROR\n";
		return nullptr;
	}
	return window;
}
