cmake_minimum_required(VERSION 2.6)
set(PROJECT_NAME src2scs)
project(${PROJECT_NAME})
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

if(MSVC)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_definitions(-DUNICODE)
add_definitions(-D_UNICODE)
find_package(Boost REQUIRED)

include_directories(
	${Boost_INCLUDE_DIR}
	${CMAKE_SOURCE_DIR}/src
)

set(PROJECT_SOURCES
	src/main.cpp
)

set(PROJECT_LIBRARIES ${PROJECT_LIBRARIES}
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
target_link_libraries(${PROJECT_NAME} ${PROJECT_LIBRARIES})
