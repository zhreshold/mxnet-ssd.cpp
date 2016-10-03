CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

#-------------------- WHAT TO CONFIG ------------------------#
SET(PROJECT_NAME "ssd")	# The project name ������
SET(LINK_OPENCV_STATIC ON)	# Use static library or not �Ƿ����Ӿ�̬���п�
SET(CUSTOM_OPENCV_DIR "D:\\Dev\\tmp\\opencv\\build") # where OpenCV reside in OpenCVʵ��·��
SET(USE_CUSTOM_OPENCV_DIR_IN_NIX OFF) # Most likely you don't need to specify it in *nix
#------------------------------------------------------------#







#-------------------- NO MIDIFICATION ------------------------#
PROJECT(${PROJECT_NAME})
IF(${LINK_OPENCV_STATIC})
	SET(OpenCV_STATIC ON)
	IF ( MSVC )
		SET ( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd" )
		SET ( CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT" )
		SET ( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd" )
		SET ( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT" )
	ENDIF ( MSVC )
ELSE()
	SET(OpenCV_STATIC OFF)
ENDIF(${LINK_OPENCV_STATIC})

IF(WIN32)
	SET(OpenCV_DIR ${CUSTOM_OPENCV_DIR})
ELSE()
	IF(${USE_CUSTOM_OPENCV_DIR_IN_NIX})
		SET(OpenCV_DIR ${CUSTOM_OPENCV_DIR}) # set dir in unix only if you required
	ENDIF(${USE_CUSTOM_OPENCV_DIR_IN_NIX})
ENDIF(WIN32)

FIND_PACKAGE(OpenCV REQUIRED)
FILE(GLOB ALL_HEADERS "../src/*.h*")
FILE(GLOB ALL_SOURCES "../src/*.c*")
INCLUDE_DIRECTORIES(${OpenCV_INCLUDE_DIRS} "../include")

ADD_EXECUTABLE(${PROJECT_NAME} ${ALL_HEADERS} ${ALL_SOURCES})
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} ${OpenCV_LIBS} )

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
        message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()
