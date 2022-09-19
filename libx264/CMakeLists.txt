cmake_minimum_required(VERSION 3.0)
aux_source_directory(. DIR_LIB_SRCS)
include_directories(include)
link_directories(libx264-x64)
add_library(libx264Utils STATIC ${DIR_LIB_SRCS})
target_link_libraries(libx264Utils
						PRIVATE
						libx264)