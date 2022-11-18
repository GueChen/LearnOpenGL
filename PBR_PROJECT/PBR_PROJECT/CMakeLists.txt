# CMakeList.txt: PBR_PROJECT 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

set(TARGET_NAME pbr_demo)

file(GLOB_RECURSE HEADER_FILES "*.h")
file(GLOB_RECURSE SOURCE_FILES "*.cpp" "*.c")
# add glfw3 library
find_package(glfw3 CONFIG REQUIRED)

add_executable (${TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES})

set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 20)

# add dependencies
if(glfw3_FOUND)
	target_include_directories(${TARGET_NAME} PUBLIC ${GLFW3_INCLUDE_DIR})
	target_link_libraries(${TARGET_NAME} PUBLIC glfw)
else(glfw3_FOUND)
	message(FATAL_ERROR "COUDNT FOUND GLFW3 LIB")
endif(glfw3_FOUND)

# link the common library
target_link_libraries(${TARGET_NAME} PUBLIC common_lib)