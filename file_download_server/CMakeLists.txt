cmake_minimum_required (VERSION 2.8 FATAL_ERROR)

set(PROJECT "project")

project (${PROJECT})

# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_SRCS 变量
aux_source_directory(. DIR_SRCS)
aux_source_directory(../../ztl/src/network DIR_SRCS)
aux_source_directory(../../ztl/src/utils DIR_SRCS)
list(REMOVE_ITEM DIR_SRCS ../../ztl/src/network/http_header_test.cc)

find_package(OpenCV REQUIRED)
find_package(Threads)

include_directories("../../ztl/src")
include_directories( ${OpenCV_INCLUDE_DIRS} )

# 指定生成目标
add_executable(${PROJECT} ${DIR_SRCS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++11")
# Generate extra compilation info used by YCM.
set( CMAKE_EXPORT_COMPILE_COMMANDS 1 )

target_link_libraries (${PROJECT} pthread ${OpenCV_LIBS})
