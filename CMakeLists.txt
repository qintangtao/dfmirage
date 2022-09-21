cmake_minimum_required(VERSION 3.0)

SET (CMAKE_C_COMPILER             	"gcc")
SET (CMAKE_C_FLAGS                	"-Wall -std=c99")
SET (CMAKE_C_FLAGS_DEBUG          	"-g")
SET (CMAKE_C_FLAGS_MINSIZEREL     	"-Os -DNDEBUG")
SET (CMAKE_C_FLAGS_RELEASE        	"-O4 -DNDEBUG")
SET (CMAKE_C_FLAGS_RELWITHDEBINFO 	"-O2 -g")
 
SET (CMAKE_CXX_COMPILER             "g++")
SET (CMAKE_CXX_FLAGS                "-Wall -std=gnu++11")
SET (CMAKE_CXX_FLAGS_DEBUG          "-g")
SET (CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELEASE        "-O4 -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

SET (CMAKE_AR      "ar")
SET (CMAKE_LINKER  "ld")
SET (CMAKE_NM      "nm")
SET (CMAKE_OBJDUMP "objdump")
SET (CMAKE_RANLIB  "ranlib")

SET (CMAKE_BUILD_TYPE Release)


SET (OUTPUT_NAME EasyScreenCapture)

# unknown error
#PROJECT (${OUTPUT_NAME})

SET (CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/bin64)


aux_source_directory(. SRC_FILE)
aux_source_directory(./thread SRC_FILE4)
aux_source_directory(./exception SRC_FILE5)

include_directories(${CMAKE_SOURCE_DIR}
					thread
					exception
					IPCamera64/include
					ffmpeg64/include
					libyuv/include
					libx264/libx264-x64/include
					jrtplib/include)

link_directories(libx264/libx264-x64/lib
					IPCamera64/lib
					ffmpeg64/lib
					libyuv/lib
					jrtplib/lib64)

add_executable(${OUTPUT_NAME} ${SRC_FILE} ${SRC_FILE4} ${SRC_FILE5})

target_compile_definitions(${OUTPUT_NAME}
                            PRIVATE
                            ENABLED_FFMPEG_ENCODER		# 启用ffmpeg编码
                            #ENABLED_FFMPEG_LOG			# 启用ffmpeg日志
                            ENABLED_TEST_FPS			# 计算FPS
                            ENABLED_LIBYUV_CONVERT		# 使用libyuv转换(RGB32->YV12)
                            )

add_subdirectory(libyuv)
add_subdirectory(libx264)
add_subdirectory(jrtplib)

target_link_libraries(${OUTPUT_NAME}
						PRIVATE
						gdi32
						ws2_32
						libEasyIPCamera
						avcodec
						avutil
						swscale
						yuvUtils
						libx264Utils
						jrtpUtils)