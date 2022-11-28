cmake_minimum_required (VERSION 3.8)

set(TARGET_NAME common_lib)

file(GLOB_RECURSE HEADER_FILES "inc/*.h" "inc/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "src/*.c" "src/*.cpp")
# add assimp library
find_package(assimp CONFIG REQUIRED)

# add opengl 
find_package(glad CONFIG REQUIRED)

# add glm
find_package(GLM CONFIG REQUIRED)

# find stb
find_path(STB_INCLUDE_DIRS "stb_c_lexer.h")

#
find_package(glfw3 CONFIG REQUIRED)

add_library(${TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES})

set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${TARGET_NAME} PROPERTY LINKER_LANGUAGE CXX)

if(glad_FOUND)
	target_include_directories(${TARGET_NAME} PUBLIC ${GLAD_INCLUDE_DIR})
	target_link_libraries(${TARGET_NAME} PUBLIC glad::glad)
else()
	message(FATAL_ERROR "COUDNT FOUND GLAD LIB")
endif()

if(GLM_FOUND)
	message(STATUS "GLM Have Found - {GLM_INCLUDE_DIR} = ${GLM_INCLUDE_DIR}")
	target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIR})
	target_link_libraries(${TARGET_NAME} PUBLIC glm::glm)
else()
	message(FATAL_ERROR "COUDNT FOUND GLM LIB")
endif()

if(assimp_FOUND)
	target_include_directories(${TARGET_NAME} PUBLIC ${ASSIMP_INCLUDE_DIR})
	target_link_libraries(${TARGET_NAME} PUBLIC assimp::assimp)
else()
	message(FATAL_ERROR "COUDNT FOUND ASSIMP LIB")
endif()

target_include_directories(${TARGET_NAME} PUBLIC ${STB_INCLUDE_DIRS})

if(glfw3_FOUND)
	target_include_directories(${TARGET_NAME} PUBLIC ${GLFW3_INCLUDE_DIR})
	target_link_libraries(${TARGET_NAME} PUBLIC glfw)
else()
	message(FATAL_ERROR "COUDNT FOUND GLFW3 LIB")
endif()

target_include_directories(
	${TARGET_NAME} 
	PUBLIC
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>"
)