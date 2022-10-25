include(../../qtcreatorplugin.pri)
QT += concurrent gui-private network printsupport serialport qml
HEADERS += openmvplugin.h \
           openmveject.h \
           openmvpluginserialport.h \
           openmvpluginio.h \
           openmvpluginfb.h \
           openmvterminal.h \
           openmvcamerasettings.h \
           openmvdataseteditor.h \
           histogram/openmvpluginhistogram.h \
           tools/bossac.h \
           tools/dfu-util.h \
           tools/picotool.h \
           tools/edgeimpulse.h \
           tools/thresholdeditor.h \
           tools/keypointseditor.h \
           tools/tag16h5.h \
           tools/tag25h7.h \
           tools/tag25h9.h \
           tools/tag36h10.h \
           tools/tag36h11.h \
           tools/tag36artoolkit.h \
           tools/videotools.h \
           qcustomplot/qcustomplot.h \
           k210_burn/k210burndialog.h \
           k210_burn/download/k210burn.h \
           k210_burn/download/k210burnRam.h \
           k210_burn/download/k210burnFlash.h \
           k210_burn/download/util.h \
           k210_burn/custom_combobox/custom_combobox.h
SOURCES += openmvplugin.cpp \
           openmveject.cpp \
           openmvpluginserialport.cpp \
           openmvpluginio.cpp \
           openmvpluginfb.cpp  \
           openmvterminal.cpp \
           openmvcamerasettings.cpp \
           openmvdataseteditor.cpp \
           histogram/openmvpluginhistogram.cpp \
           histogram/rgb2rgb_tab.c \
           histogram/lab_tab.c \
           histogram/yuv_tab.c \
           tools/bossac.cpp \
           tools/dfu-util.cpp \
           tools/picotool.cpp \
           tools/edgeimpulse.cpp \
           tools/thresholdeditor.cpp \
           tools/keypointseditor.cpp \
           tools/tag16h5.c \
           tools/tag25h7.c \
           tools/tag25h9.c \
           tools/tag36h10.c \
           tools/tag36h11.c \
           tools/tag36artoolkit.c \
           tools/videotools.cpp \
           qcustomplot/qcustomplot.cpp \
           k210_burn/k210burndialog.cpp \
           k210_burn/download/k210burn.cpp \
           k210_burn/download/k210burnRam.cpp \
           k210_burn/download/k210burnFlash.cpp \
           k210_burn/download/util.cpp \
           k210_burn/custom_combobox/custom_combobox.cpp
FORMS += openmvcamerasettings.ui histogram/openmvpluginhistogram.ui k210_burn/k210burndialog.ui
RESOURCES += openmv.qrc
