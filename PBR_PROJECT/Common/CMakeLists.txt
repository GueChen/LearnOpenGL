cmake_minimum_required (VERSION 3.8)

set(TARGET_NAME common_lib)

file(GLOB_RECURSE HEADER_FILES "inc/*.h")
file(GLOB_RECURSE SOURCE_FILES "src/*.c" "src/*.cpp")
# add assimp library
find_package(assimp CONFIG REQUIRED)

# add opengl 
find_package(glad CONFIG REQUIRED)

# add glm
find_package(glm CONFIG REQUIRED)

# find stb
find_path(STB_INCLUDE_DIRS "stb_c_lexer.h")


add_library(${TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES})

set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${TARGET_NAME} PROPERTY LINKER_LANGUAGE CXX)

if(glad_FOUND)
	target_include_directories(${TARGET_NAME} PUBLIC ${GLAD_INCLUDE_DIR})
	target_link_libraries(${TARGET_NAME} PUBLIC glad::glad)
else(glad_FOUND)
	message(FATAL_ERROR "COUDNT FOUND GLAD LIB")
endif(glad_FOUND)

if(glm_FOUND)
	target_include_directories(${TARGET_NAME} PUBLIC ${GLM_INCLUDE_DIR})
	target_link_libraries(${TARGET_NAME} PUBLIC glm::glm)
else(glm_FOUND)
	message(FATAL_ERROR "COUDNT FOUND GLM LIB")
endif(glm_FOUND)

if(assimp_FOUND)
	target_include_directories(${TARGET_NAME} PUBLIC ${assimp_INCLUDE_DIR})
	target_link_libraries(${TARGET_NAME} PUBLIC assimp::assimp)
else(assimp_FOUND)
	message(FATAL_ERROR "COUDNT FOUND ASSIMP LIB")
endif(assimp_FOUND)

target_include_directories(${TARGET_NAME} PUBLIC ${STB_INCLUDE_DIRS})

target_include_directories(
	${TARGET_NAME} 
	PUBLIC
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>"
)