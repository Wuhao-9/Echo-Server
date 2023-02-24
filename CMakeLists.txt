cmake_minimum_required(VERSION 3.8)

project(Reactor-Echo)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/bin")

file(GLOB SRCS "${CMAKE_SOURCE_DIR}/*.cpp")

add_executable(${CMAKE_PROJECT_NAME} ${SRCS})

target_link_libraries(${CMAKE_PROJECT_NAME} pthread)