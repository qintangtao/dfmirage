QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

TARGET = EasyIPCamera_RTSP
# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DEFINES += ENABLED_FFMPEG_ENCODER

CONFIG(debug, debug|release) {

}else{
    MOC_DIR = $$PWD/temp/moc
    RCC_DIR = $$PWD/temp/rcc
    UI_DIR = $$PWD/temp/ui
    OBJECTS_DIR = $$PWD/temp/obj
    DESTDIR = $$PWD/bin
}

SOURCES += main.cpp \
    RegistryKey.cpp \
    Decoder/H264Encoder.cpp \
    MirrorDriverClient.cpp \
    Exception.cpp \
    StringStorage.cpp \
    PixelFormat.cpp


HEADERS += \
    RegistryKey.h \
    DisplayEsc.h \
    Decoder/H264Encoder.h \
    MirrorDriverClient.h \
    Exception.h \
    StringStorage.h \
    PixelFormat.h \
    Dimension.h \
    Point.h \
    Rect.h

win32: LIBS += -lgdi32 -llibws2_32

LIBS += -L$$PWD/Decoder/lib/ -llibx264 \
               -L$$PWD/IPCamera/lib/ -llibEasyIPCamera

#LIBS += -L$$PWD/ffmpeg/lib/ -lavcodec -lavdevice -lavfilter -lavformat  -lavutil -lpostproc -lswresample -lswscale
LIBS += -L$$PWD/ffmpeg/lib/ -lavcodec -lavutil -lswscale

INCLUDEPATH += $$PWD/Decoder/include \
               $$PWD/IPCamera/include \
               $$PWD/ffmpeg/include
