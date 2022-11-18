# CMakeList.txt: PBR_PROJECT 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

set(TARGET_NAME pbr_demo)

file(GLOB_RECURSE HEADER_FILES "*.h")
file(GLOB_RECURSE SOURCE_FILES "*.cpp" "*.c")
# add glfw3 library
find_package(glfw3 CONFIG REQUIRED)
# add assimp library
find_package(assimp CONFIG REQUIRED)
# add opengl 
find_package(glad CONFIG REQUIRED)
# add glm
find_package(glm CONFIG REQUIRED)
# find stb
find_path(STB_INCLUDE_DIRS "stb_c_lexer.h")

add_executable (${TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES})

set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 20)

# add dependencies
if(glfw3_FOUND)
	target_include_directories(${TARGET_NAME} PUBLIC ${GLFW3_INCLUDE_DIR})
	target_link_libraries(${TARGET_NAME} PUBLIC glfw)
else(glfw3_FOUND)
	message(FATAL_ERROR "COUDNT FOUND GLFW3 LIB")
endif(glfw3_FOUND)

if(assimp_FOUND)
	target_include_directories(${TARGET_NAME} PUBLIC ${assimp_INCLUDE_DIR})
	target_link_libraries(${TARGET_NAME} PUBLIC assimp::assimp)
else(assimp_FOUND)
	message(FATAL_ERROR "COUDNT FOUND ASSIMP LIB")
endif(assimp_FOUND)

if(glad_FOUND)
	target_include_directories(${TARGET_NAME} PUBLIC ${GLAD_INCLUDE_DIR})
	target_link_libraries(${TARGET_NAME} PUBLIC glad::glad)
else(glad_FOUND)
	message(FATAL_ERROR "COUDNT FOUND GLAD LIB")
endif(glad_FOUND)

if(glm_FOUND)
	target_include_directories(${TARGET_NAME} PUBLIC ${glm_INCLUDE_DIR})
	target_link_libraries(${TARGET_NAME} PUBLIC glm::glm)
else(glm_FOUND)
	message(FATAL_ERROR "COUDNT FOUND GLM LIB")
endif(glm_FOUND)

target_include_directories(${TARGET_NAME} PRIVATE ${STB_INCLUDE_DIRS})