#--------------------------------------------------------------------
# FS-MP1A 像素马里奥 —— 收官番外篇
# 编译（Ubuntu，用 buildroot 的 qmake，别用系统 qmake）：
#   <buildroot源码>/output/host/bin/qmake fsmp1a-mario.pro
#   make -j2
# 板上运行（出厂内核 + 新根）：
#   /opt/fsmp1a-mario -platform linuxfb
#--------------------------------------------------------------------
QT       += widgets
CONFIG   += c++11
TEMPLATE  = app
TARGET    = fsmp1a-mario

SOURCES  += main.cpp
HEADERS  += pixelart.h
