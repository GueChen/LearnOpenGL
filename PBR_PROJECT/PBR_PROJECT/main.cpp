#include "shader.h"
#include "camera.h"
#include "model.h"
#define STB_IMAGE_IMPLEMENTATION
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
uint32_t scr_width  = 1280;
uint32_t scr_height = 720;

/*____________________________________function declarations_______________________________*/
#ifdef PBR_TEXTURE
void		InitializeTexture();
#endif // PBR_TEXTURE
void        ModifyPBRShader();
GLFWwindow* InitializeWindow();
uint32_t	InitializeIBLResource();
void		ProcessInput	(GLFWwindow* window, float delta_time);
void		RenderPass		();
void	    RenderCube(Shader& shader);
void        RenderGUI();
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
	GLFWwindow* window = InitializeWindow();	
	if (window == nullptr) {
		glfwTerminate();
		return 1;
	};
	
	ModifyPBRShader();

	uint32_t cube_map = InitializeIBLResource();
	Shader   skybox_shader(VERT_PATH(skybox), FRAG_PATH(skybox));
	
	glEnable(GL_DEPTH_TEST);
	float last_frame = 0.0f;
	while (!glfwWindowShouldClose(window)) {
		float  current_frame = static_cast<float>(glfwGetTime());
		float  delta_time = current_frame - last_frame;
		last_frame		  = current_frame;

		ProcessInput(window, delta_time);

		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		RenderPass  ();
		
		glDepthFunc(GL_LEQUAL);
		skybox_shader.use();
		skybox_shader.setInt("env_map", 0);
		skybox_shader.setMat4("proj", glm::perspective(glm::radians(camera.Zoom), (float)(scr_width)/ (float)(scr_height), 0.1f, 100.0f));
		skybox_shader.setMat4("view", camera.GetViewMatrix());
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cube_map);
		RenderCube(skybox_shader);
		glDepthFunc(GL_LESS);

		RenderGUI();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}

void	 RenderSphere(Shader & shader);
pair<uint32_t, uint32_t> 
		 InitSphereResource();
uint32_t InitCubeResource();

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
	pbr_shader.setMat4("proj",  glm::perspective(glm::radians(camera.Zoom), (float)scr_width / scr_height, 0.1f, 100.0f));
	pbr_shader.setMat4("view",  camera.GetViewMatrix());

	// set global fragment properties
	pbr_shader.setVec3("light_pos",	  m_light.pos);
	pbr_shader.setVec3("light_color", m_light.color);
	pbr_shader.setVec3("camera_pos",  camera.Position);
	
	RenderSphere(pbr_shader);

	
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

void RenderCube(Shader& shader)
{
	static uint32_t cube_vao = 0;

	if (0 == cube_vao) {
		cube_vao = InitCubeResource();
	}

	glBindVertexArray(cube_vao);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
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
	ImGui::SliderFloat("metalic",   &metalic, 0.0f, 1.0f, "%.2f");
	ImGui::SliderFloat("roughness", &roughness, 0.05f, 1.0f, "%.2f");
	ImGui::ColorPicker3("albedo",   glm::value_ptr(albedo));
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

uint32_t InitCubeResource()
{
	float vertices[] = {
		// back face
		-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
		 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
		 1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
		 1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
		-1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
		-1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
		// front face
		-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
		 1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
		 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
		 1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
		-1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
		-1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
		// left face
		-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
		-1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
		-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
		-1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
		-1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
		-1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
		// right face
		 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
		 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
		 1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
		 1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
		 1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
		 1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
		// bottom face
		-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
		 1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
		 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
		 1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
		-1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
		-1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
		// top face
		-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
		 1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
		 1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
		 1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
		-1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
		-1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
	};
	uint32_t vao, vbo;
	glGenVertexArrays(1, &vao);
	glGenBuffers	 (1, &vbo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindVertexArray(vao);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return vao;
}

void FrameSizeChangeCallback(GLFWwindow* window, int width, int height)
{
	scr_width  = width;
	scr_height = height;
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
		f.seekp(20, ios_base::beg);
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

	GLFWwindow* window = glfwCreateWindow(scr_width, scr_height, "PBR_EXAMPLE", NULL, NULL);
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

uint32_t
InitializeIBLResource()
{
	static uint32_t ibl_width = 1024, ibl_height = 1024;
	// fbo and rbo idx
	uint32_t fbo, rbo;
	glGenFramebuffers(1,  &fbo);
	glGenRenderbuffers(1, &rbo);

	glBindFramebuffer(GL_FRAMEBUFFER,   fbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, ibl_width, ibl_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

	// cube map idx
	uint32_t env_cubemap;
	glGenTextures(1, &env_cubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, env_cubemap);
	for (uint32_t i = 0; i < 6; ++i) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, ibl_width, ibl_height, 0, GL_RGB, GL_FLOAT, nullptr);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	
	
	uint32_t hdr_texture = 0;
	stbi_set_flip_vertically_on_load(true);
	int width, height, nr_components;
	float* data = stbi_loadf(ASSET_PATH_DIR"/sunsetpeek/sunsetpeek_ref.hdr", &width, &height, &nr_components, 0);
	if (data){
		glGenTextures(1, &hdr_texture);
		glBindTexture(GL_TEXTURE_2D, hdr_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else {
		cout << "Failed to load HDR image\n";
	}

	Shader et2cube_shader(VERT_PATH(ira_ibl), FRAG_PATH(ira_ibl));	
	glm::mat4 cap_proj       = glm::perspective(glm::radians(90.0f), 1.0f, 0.0f, 10.0f);
	glm::mat4 cap_views[] = {
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f),  glm::vec3(0.0f, -1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f,  0.0f),  glm::vec3(0.0f, -1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f),  glm::vec3(0.0f,  0.0f, 1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  -1.0f, 0.0f),  glm::vec3(0.0f,  0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f),  glm::vec3(0.0f, -1.0f, 0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
	};

	et2cube_shader.use();
	et2cube_shader.setInt("equirectangular_map", 0);
	et2cube_shader.setMat4("proj", cap_proj);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, hdr_texture);
	
	glViewport(0, 0, ibl_width, ibl_height);
	for (uint32_t i = 0; i < 6; ++i) {
		et2cube_shader.setMat4("view", cap_views[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, env_cubemap, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		RenderCube(et2cube_shader);
	}
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1,  &fbo);
	glDeleteRenderbuffers(1, &rbo);
	glDeleteTextures(1, &hdr_texture);
	glViewport(0, 0, scr_width, scr_height);
	return env_cubemap;
}
