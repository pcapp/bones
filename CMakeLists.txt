cmake_minimum_required(VERSION 2.8)
project(bones)

set(INCLUDES MD5Reader.h AnimCore.h)
set(SHADERS simple.vert simple.frag)
source_group(Shaders FILES simple.vert simple.frag)
set(SRCS main.cpp MD5Reader.cpp ${SHADERS})


# For Visual Studio
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(GLEW REQUIRED)
find_package(GLM REQUIRED)

include_directories(${OPENGL_INCLUDE_DIR})
include_directories(${GLUT_INCLUDE_DIR})
include_directories(${GLEW_INCLUDE_PATH})
include_directories(${GLM_INCLUDE_DIRS})

if(CMAKE_COMPILER_IS_GNUXX) 
	set(CMAKE_CXX_FLAGS "-std=c++11")
endif(CMAKE_COMPILER_IS_GNUXX)

add_executable(main ${SRCS} ${INCLUDES})
target_link_libraries(main ${GLUT_LIBRARIES} ${OPENGL_LIBRARY} ${GLEW_LIBRARY})

# Copy the shaders so:
# 1. VS can find them
# 2. The .exe has access to them
foreach(SHADER ${SHADERS}) 
	add_custom_command(TARGET main POST_BUILD 
		# Used for VS so we can 
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER} 
										 ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/
		# Place the shaders in the same directory as the executable
		COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/${SHADER} $<TARGET_FILE_DIR:main>/${SHADER}
	)
endforeach()

add_custom_command(TARGET main POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Boblamp ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Boblamp
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Boblamp $<TARGET_FILE_DIR:main>/Boblamp
)

add_executable(skeleton_test skeleton_test.cpp)