cmake_minimum_required(VERSION 3.28.3)

project(my-muduo C CXX)

# 设置编译标志
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11 ")

# 设置输出目录
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib) #库输出目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin) #可执行文件输出目录

# 包含头文件目录
include_directories(${PROJECT_SOURCE_DIR}/my_muduo)

# 编译my_muduo库
aux_source_directory(my_muduo MY_MUDUO_SRC)
add_library(my-muduo SHARED ${MY_MUDUO_SRC})

