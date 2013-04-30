cmake_minimum_required(VERSION 2.8)
project(MOVE)

set(SRCS main.cpp MD5Reader.cpp)
set(INCLUDES MD5Reader.h AnimCore.h)

# My set-up cruft. We can move this to a .bat file.
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "C:/Dev/freeglut-2.8.0/")
set(GLM_INCLUDE_DIR "C:/Dev/glm-0.9.3.4/")
# end cruft

find_package(GLUT REQUIRED)
find_package(OpenGL REQUIRED)

include_directories(${OPENGL_INCLUDE_DIR})
include_directories(${GLUT_INCLUDE_DIR})
include_directories(${GLM_INCLUDE_DIR})


if(CMAKE_COMPILER_IS_GNUXX) 
	set(CMAKE_CXX_FLAGS "-std=c++11")
endif(CMAKE_COMPILER_IS_GNUXX)

add_executable(main ${SRCS} ${INCLUDES})
target_link_libraries(main ${GLUT_LIBRARIES})

add_custom_command(TARGET main POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E echo "HELLLLP!"
	COMMAND ${CMAKE_COMMAND} -E echo "Copying to: $<TARGET_FILE_DIR:main>/Boblamp"	
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Boblamp $<TARGET_FILE_DIR:main>/Boblamp
	COMMAND ${CMAKE_COMMAND} -E echo "END POST_BUILD"
)