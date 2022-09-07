cmake_minimum_required(VERSION 3.0.0)
project(Wloger VERSION 0.1.0)

set(SRC
	Source/Wloger.cpp 
	Include/WLoger.h
)

add_library(Wloger ${SRC})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})