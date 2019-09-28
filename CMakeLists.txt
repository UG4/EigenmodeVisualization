# Copyright (c) 2008-2015:  G-CSC, Goethe University Frankfurt
# Copyright (c) 2006-2008:  Steinbeis Forschungszentrum (STZ Ölbronn)
# Copyright (c) 2006-2015:  Sebastian Reiter
# Copyright (c) 2019:  Lukas Larisch
# Author: Sebastian Reiter, Lukas Larisch
#
# This file is part of EmVis.
# 
# EmVis is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License version 3 (as published by the
# Free Software Foundation) with the following additional attribution
# requirements (according to LGPL/GPL v3 §7):
# 
# (1) The following notice must be displayed in the Appropriate Legal Notices
# of covered and combined works: "Based on ProMesh (www.promesh3d.com)".
# 
# (2) The following bibliography is recommended for citation and must be
# preserved in all covered files:
# "Reiter, S. and Wittum, G. ProMesh -- a flexible interactive meshing software
#   for unstructured hybrid grids in 1, 2, and 3 dimensions. In preparation."
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Lesser General Public License for more details.

cmake_minimum_required(VERSION 3.1)
project(P_EMVIS)

set (CMAKE_CXX_STANDARD 11)

set(EmVisSRC	src/app.cpp
				src/color_widget.cpp
				src/delegates.cpp
				src/main.cpp
				src/main_window.cpp
				src/main_window_input.cpp
				src/scene_inspector.cpp
				src/scene_item_model.cpp
				src/view3d/view3d.cpp
				src/view3d/camera/quaternion.cpp
				src/view3d/camera/model_viewer_camera.cpp
				src/view3d/camera/matrix44.cpp
				src/view3d/camera/basic_camera.cpp
				src/view3d/camera/arc_ball.cpp
				src/scene/csg_object.cpp
				src/scene/lg_object.cpp
				src/scene/lg_scene.cpp
				src/scene/lg_tmp_methods.cpp
				src/scene/plane_sphere.cpp
				src/scene/scene_interface.cpp
				src/tools/camera_tools.cpp
				src/tools/info_tools.cpp
				src/tools/oscillation_tools.cpp
				src/tools/solution_refinement_tools.cpp
				src/tools/UG_LogParser.h
				src/tools/wave_tools.cpp
				src/tools/helmholtz_tools.cpp
				src/tools/standard_tools.cpp
				src/tools/tool_dialog.cpp
				src/tools/tool_manager.cpp
				src/util/file_util.cpp
				src/util/qstring_util.cpp
				src/modules/module_interface.cpp
				src/modules/mesh_module.cpp
				src/widgets/double_slider.cpp
				src/widgets/extendible_widget.cpp
				src/widgets/file_widget.cpp
				src/widgets/icon_tab_widget.cpp
				src/widgets/property_widget.cpp
				src/widgets/tool_browser_widget.cpp
				src/widgets/truncated_double_spin_box.cpp
				src/widgets/widget_container.cpp
				src/widgets/widget_list.cpp)

# set (CMAKE_CXX_STANDARD 11)

# Some options which allow to control makefile generation
option(DEBUG "If enabled a debug build is performed - this slows down performance." OFF)
option(PROFILER "If enabled, profiling is performed - this slows down performance." OFF)
option(DEBUG_LOGS "Enables debug output. Valid options are: ON, OFF" OFF)
option(UG_ROOT_PATH "The path to ug4. Has to contain the ugbase and lib directories.")
option(QT_CMAKE_PATH "Folder containing cmake-modules for the chosen architecture.")


# add flag for c language only
function (add_c_flag flag)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}" CACHE STRING "overriden flags!" FORCE)
endfunction(add_c_flag)

# add flag for c++ language only
function (add_cpp_flag flag)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" CACHE STRING "overriden flags!" FORCE)
endfunction(add_cpp_flag)

# add flag for c and c++ language
function(add_cxx_flag flag)
	add_c_flag(${flag})
	add_cpp_flag(${flag})
endfunction(add_cxx_flag)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Cray")
	# Cray Compiler: add support for gnu extensions
	add_cxx_flag("-h gnu")
	# remove warning "The controlling expression is constant"
	add_cxx_flag("-hnomessage=236")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "XL")
    # currently no flags for IBM xl compiler
    # however, the -Wall option is not supported
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	# add_definitions(/E/c)
	# The following flag is necessary for gigantic .obj files
    # which cause problems on Windows
	add_cxx_flag("/bigobj")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	add_cxx_flag("-Wall")
	add_cxx_flag("-Wno-multichar")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	add_cxx_flag("-Wall")
	add_cxx_flag("-Wno-multichar")
    # for some reason -Wsign-compare is not in -Wall for Clang 
	add_cxx_flag("-Wsign-compare")
	add_cxx_flag(-Wno-unused-local-typedef)
	add_cxx_flag(-Wno-unknown-warning-option)
endif()


# set(Boost_USE_STATIC_LIBS   ON)
FIND_PACKAGE( Boost 1.48 COMPONENTS serialization REQUIRED system filesystem )
# INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )
add_definitions( -DBOOST_ALL_NO_LIB)
include_directories(SYSTEM ${UG_ROOT_PATH}/externals/BoostForUG4)

# Qt5 with OpenGL support is required.
# in order to use a custom installation of QT, specify QT_CMAKE_PATH when running
# cmake for the first time for your build (e.g. "...pathToQt/5.4/gcc_64/lib/cmake/")
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
if(QT_CMAKE_PATH)
	FIND_PACKAGE(Qt5Widgets REQUIRED PATHS ${QT_CMAKE_PATH}/Qt5Widgets)
	FIND_PACKAGE(Qt5OpenGL REQUIRED PATHS ${QT_CMAKE_PATH}/Qt5OpenGL)
else(QT_CMAKE_PATH)
	FIND_PACKAGE(Qt5Widgets REQUIRED)
	FIND_PACKAGE(Qt5OpenGL REQUIRED)
	if(USE_WEBKIT)
		FIND_PACKAGE(Qt5WebKitWidgets REQUIRED)
	endif(USE_WEBKIT)
endif(QT_CMAKE_PATH)


FIND_PACKAGE(OpenGL REQUIRED)

# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

include_directories(${CMAKE_SOURCE_DIR}/src)

if(DEBUG)
	message(STATUS "INFO: Debug mode. (Enable release-mode by defining -DDEBUG=OFF)")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")
	add_definitions(-DUG_DEBUG)
else(DEBUG)
	message(STATUS "INFO: Release mode. (Enable debug-mode by defining -DDEBUG=ON)")
	# Enable these flags for a fast ublas algebra
	
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG -DBOOST_UBLAS_NDEBUG -O3 -Wall -funroll-loops -ftree-vectorize -msse3 -mssse3")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNDEBUG -DBOOST_UBLAS_NDEBUG")
	if(GENERATE_DEBUG_SYMBOLS)
		message(STATUS "INFO: Generating debug symbols (-g) (Disable by defining -DGENERATE_DEBUG_SYMBOLS=OFF)")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
	endif(GENERATE_DEBUG_SYMBOLS)
	remove_definitions(-DUG_DEBUG)
endif(DEBUG)

if(DEBUG_LOGS)
	add_definitions(-DUG_ENABLE_DEBUG_LOGS)
endif(DEBUG_LOGS)

if(PROFILER)
	add_definitions(-DUG_PROFILER)
	add_definitions(-DUG_PROFILER_SHINY)
	set(UG_PROFILER ON)
	set(UG_PROFILER_SHINY ON)
endif(PROFILER)

if(NOT UG_ROOT_PATH)
	message(FATAL_ERROR "ERROR: You have to specify UG_ROOT_PATH! Add an option -DUG_ROOT_PATH=path-to-ug4")
endif(NOT UG_ROOT_PATH)

include_directories(${UG_ROOT_PATH}/ugcore/ugbase)
include_directories(${UG_ROOT_PATH}/plugins/ProMesh)
link_directories(${UG_ROOT_PATH}/lib)

# On some unix systems (e.g. ubuntu 12.04) glu.h is not automatically included.
# We thus set the define to additionally include glu.h
# This may have to be improved for broader compatibility
if(UNIX)
	if(APPLE)
		add_definitions(-DPROMESH_INCLUDE_OPENGL_GLU)
	else(APPLE)
		add_definitions(-DPROMESH_INCLUDE_GL_GLU)
	endif(APPLE)
elseif(WIN32)
	add_definitions(-DPROMESH_INCLUDE_GL_GLU)
endif(UNIX)


# resources
set(EmVisRESOURCES	EmVis.qrc)
QT5_ADD_RESOURCES(EmVisRESOURCES_RCC ${EmVisRESOURCES})

SET(allSources ${EmVisSRC} ${EmVisRESOURCES_RCC})

if(UNIX)
	if(APPLE)
		#SET(CMAKE_OSX_SYSROOT /Developer/SDKs/MacOSX10.5.sdk)
		#SET(CMAKE_OSX_DEPLOYMENT_TARGET 10.5)
		ADD_EXECUTABLE(EmVis MACOSX_BUNDLE ${allSources})
	else(APPLE)
		ADD_EXECUTABLE(EmVis ${allSources})
	endif(APPLE)
else(UNIX)
	if(MINGW)
		# resource compilation for MINGW
		ADD_CUSTOM_COMMAND( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/EmVisIcon.o
							COMMAND windres.exe -I${CMAKE_SOURCE_DIR}/deploy_data -i${CMAKE_SOURCE_DIR}/deploy_data/EmVisIcon.rc 
                             -o ${CMAKE_CURRENT_BINARY_DIR}/EmVisIcon.o )
		ADD_EXECUTABLE(EmVis WIN32 ${allSources} EmVisIcon.o)
	else(MINGW)
		ADD_EXECUTABLE(EmVis WIN32 ${allSources} ${CMAKE_SOURCE_DIR}/deploy_data/EmVisIcon.rc)
	endif(MINGW)
endif(UNIX)

set(PM_LIBS ${OPENGL_LIBRARIES} Qt5::OpenGL Qt5::Widgets grid_s tet)

TARGET_LINK_LIBRARIES(EmVis ${PM_LIBS} ${Boost_LIBRARIES})

add_custom_command(TARGET EmVis PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory tools)

add_custom_command(TARGET EmVis PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy
                   ${UG_ROOT_PATH}/bin/${tetgenFileName} $<TARGET_FILE_DIR:EmVis>/tools/${tetgenFileName})

