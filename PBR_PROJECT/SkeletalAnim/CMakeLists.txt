cmake_minimum_required (VERSION 3.8)

set(TARGET_NAME skletal_anim)

file(GLOB_RECURSE HEADER_FILES "*.h" "*.hpp")
file(GLOB_RECURSE SOURCE_FILES "*.cpp" "*.c")

# add imgui library
find_package(imgui CONFIG REQUIRED)

add_executable (${TARGET_NAME} WIN32 ${SOURCE_FILES} ${HEADER_FILES})

set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 20)

if(imgui_FOUND)
	target_include_directories(${TARGET_NAME} PUBLIC ${IMGUI_INCLUDE_DIR})
	target_link_libraries(${TARGET_NAME} PUBLIC imgui::imgui)
else()
	message(FATAL_EROR 
	"COUDNT FOUND IMGUI\n"
	"if u get vcpkg use following cmd to get download:\n"
	"vcpkg install imgui<:config>")
endif()


# link the common library
target_link_libraries(${TARGET_NAME} PUBLIC common_lib)