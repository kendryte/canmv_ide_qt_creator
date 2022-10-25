TEMPLATE = app
#OPENMV-DIFF#
#TARGET = qtcreator.sh
#OPENMV-DIFF#
TARGET = canmvide.sh
#CANMV-DIFF# TARGET += openmv.png
# TARGET += canmv.png
#OPENMV-DIFF#

include(../qtcreator.pri)

OBJECTS_DIR =

#OPENMV-DIFF#
#PRE_TARGETDEPS = $$PWD/qtcreator.sh
#OPENMV-DIFF#
PRE_TARGETDEPS = $$PWD/canmvide.sh
#CANMV-DIFF# PRE_TARGETDEPS += $$PWD/openmv.png
# PRE_TARGETDEPS += $$PWD/canmv.png
#OPENMV-DIFF#

#OPENMV-DIFF#
#QMAKE_LINK = cp $$PWD/qtcreator.sh $@ && : IGNORE REST OF LINE:
#OPENMV-DIFF#
QMAKE_LINK = cp $$PWD/canmvide.sh $@ && : IGNORE REST OF LINE:
#CANMV-DIFF# QMAKE_LINK += cp $$PWD/openmv.png $@ && : IGNORE REST OF LINE:
# QMAKE_LINK += cp $$PWD/canmv.png $@ && : IGNORE REST OF LINE:
#OPENMV-DIFF#
QMAKE_STRIP =
CONFIG -= qt separate_debug_info gdb_dwarf_index

#OPENMV-DIFF#
#QMAKE_CLEAN = qtcreator.sh
#OPENMV-DIFF#
QMAKE_CLEAN = canmvide.sh
#CANMV-DIFF# QMAKE_CLEAN += openmv.png
# QMAKE_CLEAN += canmv.png
#OPENMV-DIFF#

target.path  = $$INSTALL_BIN_PATH
INSTALLS    += target

#OPENMV-DIFF#
#DISTFILES = $$PWD/qtcreator.sh
#OPENMV-DIFF#
DISTFILES = $$PWD/canmvide.sh
#CANMV-DIFF# DISTFILES += $$PWD/openmv.png
# DISTFILES += $$PWD/canmv.png
#OPENMV-DIFF#

png.files   = $$PWD/canmv.png
png.path    = $$INSTALL_BIN_PATH
INSTALLS   += png
