#include "shader.h"
#include "camera.h"
#include "model.h"

#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <cstdint>

using namespace std;
using namespace glm;

#define VERT_PATH(name) SHADER_PATH_PREFIX#name".vert"
#define FRAG_PATH(name) SHADER_PATH_PREFIX#name".frag"

#ifdef PBR_TEXTURE
#define RUSTED_IRON_DIR ASSET_PATH_DIR"/rusted_iron"
#endif // PBR_TEXTURE

/*____________________________________const varaiable____________________________________*/
const uint32_t kScrWidth  = 1280;
const uint32_t kScrHeight = 720;

/*____________________________________function declarations_______________________________*/
#ifdef PBR_TEXTURE
void		InitializeTexture();
#endif // PBR_TEXTURE
void        ModifyPBRShader();
GLFWwindow* InitializeWindow();
void		ProcessInput	(GLFWwindow* window, float delta_time);
void		RenderPass		();

/*____________________________________global varaiable____________________________________*/ 
Camera camera(vec3(0.0f, 0.0f, 3.0f));
bool   rotate_flag = false;

#ifdef PBR_TEXTURE
uint32_t albedo    = 0;
uint32_t normal    = 0;
uint32_t metallic  = 0;
uint32_t roughness = 0;
uint32_t ao		   = 0;
#else
float  metalic   = 0.05f;
float  roughness = 0.05f;
vec3   albedo    = vec3(0.5f, 0.0f, 0.0f);
#endif

int main()
{	
	GLFWwindow* window	   = InitializeWindow();
	float       last_frame = 0.0f;

	if (window == nullptr) goto program_end;
	
	ModifyPBRShader();

	glEnable(GL_DEPTH_TEST);

	while (!glfwWindowShouldClose(window)) {
		float  current_frame = static_cast<float>(glfwGetTime());
		float  delta_time = current_frame - last_frame;
		last_frame = current_frame;

		ProcessInput(window, delta_time);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		RenderPass  ();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

program_end:
	glfwTerminate();
	return 0;
}

void	 RenderSphere(Shader & shader);
void	 RenderGUI();
pair<uint32_t, uint32_t> 
		 InitSphereResource();

void RenderPass()
{
	struct Light {
		vec3 pos;
		vec3 color;
	};
	// scene relate global obj
	Light m_light{.pos   = {10.0f, 0.0f, 10.0f},
				  .color = {300.0f, 300.0f, 300.0f}};

	static Shader pbr_shader(VERT_PATH(pbr), FRAG_PATH(pbr));

	pbr_shader.use();

#ifdef PBR_TEXTURE
	static bool   shader_first_use = true;
	if (shader_first_use) {
		pbr_shader.setInt("albedo_map",	   0);
		pbr_shader.setInt("normal_map",    1);
		pbr_shader.setInt("metallic_map",  2);
		pbr_shader.setInt("roughness_map", 3);
		pbr_shader.setInt("ao_map",		   4);
		shader_first_use = false;

		InitializeTexture();
	}
#endif // PBR_TEXTURE

	
	// set global vert properties
	pbr_shader.setMat4("proj",  glm::perspective(glm::radians(camera.Zoom), (float)kScrWidth / kScrHeight, 0.1f, 100.0f));
	pbr_shader.setMat4("view",  camera.GetViewMatrix());

	// set global fragment properties
	pbr_shader.setVec3("light_pos",	  m_light.pos);
	pbr_shader.setVec3("light_color", m_light.color);
	pbr_shader.setVec3("camera_pos",  camera.Position);
	
	RenderSphere(pbr_shader);

	RenderGUI();
}

void RenderSphere(Shader & shader)
{
	static uint32_t sphere_vao  = 0;
	static uint32_t index_count = 0;

	if (0 == sphere_vao) {
		std::tie(sphere_vao, index_count) = InitSphereResource();
	}
	
	// vertex attribution
	shader.setMat4 ("model",     glm::mat4(1.0f));

	// fragment attribution
#ifdef PBR_TEXTURE
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, albedo);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, metallic);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, roughness);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, ao);
#else
	shader.setVec3("albedo", albedo);
	shader.setFloat("metallic", metalic);
	shader.setFloat("roughness", roughness);
	shader.setFloat("ao", 1.0f);
#endif // PBR_TEXURE
		
	glBindVertexArray(sphere_vao);
	glDrawElements(GL_TRIANGLE_STRIP, index_count, GL_UNSIGNED_INT, 0);

}

void RenderGUI()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	ImGui::NewFrame();
	ImGui::Begin("Setting Box");
	ImGui::Text("Properties");

#ifdef PBR_TEXTURE
	
#else
	ImGui::SliderFloat("metalic", &metalic, 0.0f, 1.0f, "%.2f");
	ImGui::SliderFloat("roughness", &roughness, 0.05f, 1.0f, "%.2f");
	ImGui::ColorPicker3("albedo", glm::value_ptr(albedo));
#endif // PBR_TEXTURE
	
	ImGui::End();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

}

pair<uint32_t, uint32_t> InitSphereResource()
{
	uint32_t bo[2];
	glGenBuffers(2, &bo[0]);

	vector<vec3>	 pos;
	vector<vec2>	 uv;
	vector<vec3>	 norms;
	vector<uint32_t> indices;

	const uint32_t kXSegments = 64;
	const uint32_t kYSegments = 64;
	const float	   kPI		  = 3.15159265359f;
	for(uint32_t x = 0; x <= kXSegments; ++x)
	for(uint32_t y = 0; y <= kYSegments; ++y) {
		float x_seg = (float)x / (float)kXSegments;
		float y_seg = (float)y / (float)kYSegments;

		float x_pos = cos(x_seg * 2.0f * kPI) * sin(y_seg * kPI);
		float y_pos = cos(y_seg * kPI);
		float z_pos = sin(x_seg * 2.0f * kPI) * sin(y_seg * kPI);

		pos.emplace_back(x_pos, y_pos, z_pos);
		uv.emplace_back(x_seg, y_seg);
		norms.emplace_back(x_pos, y_pos, z_pos);
	}

	bool odd_row = false;
	for (uint32_t y = 0; y < kYSegments; ++y) {
		if (!odd_row) {
			for (uint32_t x = 0; x <= kXSegments; ++x) {
				indices.push_back(y		 * (kXSegments + 1) + x);
				indices.push_back((y + 1)* (kXSegments + 1) + x);
			}
		}
		else {
			for (int32_t x = kXSegments; x >= 0; --x) {
				indices.push_back((y + 1) * (kXSegments + 1) + x);
				indices.push_back(y		  * (kXSegments + 1) + x);
			}
		}
		odd_row = !odd_row;
	}
		
	vector<float> data;
	for (uint32_t i = 0; i < pos.size(); ++i) {
		data.push_back(pos[i].x);
		data.push_back(pos[i].y);
		data.push_back(pos[i].z);

		if (norms.size() > 0) {
			data.push_back(norms[i].x);
			data.push_back(norms[i].y);
			data.push_back(norms[i].z);
		}

		if (uv.size() > 0) {
			data.push_back(uv[i].x);
			data.push_back(uv[i].y);
		}
	}

	uint32_t vao;

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, bo[0]);
	glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bo[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), &indices[0], GL_STATIC_DRAW);
	uint32_t stride = (3 + 3 + 2) * sizeof(float);
	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
	
	return {vao, static_cast<uint32_t>(indices.size())};
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
	
	if (rotate_flag) {
		camera.ProcessPerspectiveView(xoffset, -yoffset);
	}
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

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS) {
		rotate_flag = true;
	}
	else {
		rotate_flag = false;
	}
	
}

#ifdef PBR_TEXTURE
void InitializeTexture()
{
	albedo    = TextureFromFile("albedo.png",    RUSTED_IRON_DIR);
	normal    = TextureFromFile("normal.png",    RUSTED_IRON_DIR);
	metallic  = TextureFromFile("metallic.png",  RUSTED_IRON_DIR);
	roughness = TextureFromFile("roughness.png", RUSTED_IRON_DIR);
	ao		  = TextureFromFile("ao.png",		 RUSTED_IRON_DIR);
}
#endif // PBR_TEXTURE

void ModifyPBRShader()
{
	fstream f;
	f.open(FRAG_PATH(pbr), ios::in | ios::binary | ios::out);
	if (f.is_open()) {
		f.seekp(19, ios_base::beg);
#ifdef PBR_TEXTURE
		f << "#define PBR_TEXTURE";
#else
		f << "                   ";
#endif
	}
	else {
		cout << "File open failure, some error occur\n";
	}

	f.close();
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
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	// loading glad
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		cout << "GLAD Initialize ERROR\n";
		return nullptr;
	}

	ImGui::CreateContext();
	ImGui::StyleColorsClassic();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 450");

	return window;
}
