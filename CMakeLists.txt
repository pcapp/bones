cmake_minimum_required(VERSION 2.8)
project(bones)

set(SRCS main.cpp MD5Reader.cpp)
set(INCLUDES MD5Reader.h AnimCore.h)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(GLM REQUIRED)
find_package(GLEW REQUIRED)

include_directories(${GLEW_INCLUDE_PATH})
include_directories(${OPENGL_INCLUDE_DIR})
include_directories(${GLUT_INCLUDE_DIR})
include_directories(${GLM_INCLUDE_DIRS})

if(CMAKE_COMPILER_IS_GNUXX) 
	set(CMAKE_CXX_FLAGS "-std=c++11")
endif(CMAKE_COMPILER_IS_GNUXX)

add_executable(main ${SRCS} ${INCLUDES})
target_link_libraries(main ${GLUT_LIBRARIES} ${OPENGL_LIBRARY} ${GLEW_LIBRARY})

add_custom_command(TARGET main POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Boblamp $<TARGET_FILE_DIR:main>/Boblamp
)

add_executable(skeleton_test skeleton_test.cpp)