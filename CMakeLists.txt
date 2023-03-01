project(single-reactor)

add_executable(single-reactor main.cpp)

target_link_libraries(single-reactor pthread)
