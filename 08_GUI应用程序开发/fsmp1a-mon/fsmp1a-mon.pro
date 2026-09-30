#--------------------------------------------------------------------
# FS-MP1A 板上系统监视器 —— 番外篇之二
# 编译（Ubuntu，用 buildroot 的 qmake，别用系统 qmake）：
#   <buildroot源码>/output/host/bin/qmake fsmp1a-mon.pro
#   make -j2
# 板上运行（出厂内核 + 新根）：
#   /opt/fsmp1a-mon -platform linuxfb
#   可选 --mode=man|flow|blink|disk|follow 直接进入某模式
#--------------------------------------------------------------------
QT       += widgets
CONFIG   += c++11
TEMPLATE  = app
TARGET    = fsmp1a-mon

SOURCES  += main.cpp
HEADERS  += font5x7.h
