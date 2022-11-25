#include "default_callback.h"
#include "camera.h"

extern Camera camera;

bool   rotate_flag = false;
int    scr_width   = 800;
int    scr_height  = 600;

void DftScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void DftFrameSizeChangeCallback(GLFWwindow* window, int width, int height)
{
	scr_width  = width;
	scr_height = height;
	glViewport(0, 0, width, height);
}

void DftCursorPosCallback(GLFWwindow* window, double xpos, double ypos)
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
