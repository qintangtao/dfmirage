# 简化第一版
# OBJS   	代替  依赖文件
# CC     	代替  gcc
# CFLAGS 	代替  编译命令

# 简化第二版
# $^ 		代替 构造所需文件列表所有文件的名字
# $< 		代替 构造所需文件列表第一个文件的名字
# RM 		代替 rm -f
# $@ 		代替 目标文件

# 简化第三版
# 6-11行代码相似性很强，可以提取出一个公式模版
#      %.o:%.c
#	       $(CC) $^ $(CFLAGS)  -o $@
# 百分号相当于一个通配符

PREFIX 		= ../dfmirage/bin64
INCLUDES	= -I../dfmirage -I. -I./thread -I./exception -I../dfmirage/Decoder/libx264-x64/include -I../dfmirage/IPCamera64/include -I../dfmirage/ffmpeg64/include -Imingw64/include
DEFINES		= -DTEST_FPS -DUNICODE -D_UNICODE -DWIN32 -DMINGW_HAS_SECURE_API=1
# 启用ffmpeg编码
DEFINES		+= -DENABLED_FFMPEG_ENCODER
# 启用ffmpeg日志
#DEFINES		+= -DENABLED_FFMPEG_LOG
# 计算FPS
DEFINES		+= -DTEST_FPS
LINK_OPTS 	= -L../dfmirage/Decoder/libx264-x64/lib -L../dfmirage/IPCamera64/lib -L../dfmirage/ffmpeg64/lib -Lmingw32/i686-w64-mingw32/lib/
LINK_LIBS 	= -lgdi32 -lws2_32 -llibx264 -llibEasyIPCamera -lavcodec -lavutil -lswscale
CC 			= g++
CFLAGS 		= -c -fno-keep-inline-dllexport -O2 -std=gnu++11 -Wall -W -Wextra -fexceptions -mthreads $(DEFINES) $(INCLUDES)
EXE_NAME 	= EasyScreenCapture



SRC  := $(wildcard *.cpp Decoder/*.cpp thread/*.cpp exception/*.cpp)
OBJS  := $(SRC:%.cpp=%.o)

#all:
#	@echo $(SRC)

$(EXE_NAME):$(OBJS)
	$(CC) $^ -o $@ $(LINK_OPTS) $(LINK_LIBS)

%.o:%.cpp
	$(CC) $< $(CFLAGS) -o $@

#main.o:main.c
#	$(CC) $^ $(CFLAGS) -o $@
#sem_comm.o:sem_comm.c
#	$(CC) $^ $(CFLAGS) -o $@

install: $(EXE_NAME)
	cp -r $(EXE_NAME) $(PREFIX)

clean:
	$(RM) *.o $(EXE_NAME) -r