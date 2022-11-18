# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.8)

if(DEFINED ENV{VCPKG})
message("vcpkg found install path = $ENV{VCPKG}")
set(VCPKG_ROOT "$ENV{VCPKG}\\scripts\\buildsystems\\vcpkg.cmake")
set(CMAKE_TOOLCHAIN_FILE ${VCPKG_ROOT})
else()
message(FATAL_ERROR 
		"vcpkg not exist, please set it to environment variables\n"
		"format - VCPKG = <path>\n"
		"Windows Powershell user:\n"
		"$Env:VCPKG = '{vcpkg install path}'"
)
endif()

project ("OPGL_PROJ")

# 包含子项目。
add_subdirectory ("PBR_PROJECT")
