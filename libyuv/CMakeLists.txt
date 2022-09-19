cmake_minimum_required(VERSION 3.0)
aux_source_directory(. DIR_LIB_SRCS)
include_directories(include)
link_directories(ib)
add_library(yuvUtils STATIC ${DIR_LIB_SRCS})
target_link_libraries(yuvUtils
						PRIVATE
						libyuv)