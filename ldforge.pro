######################################################################
# Automatically generated by qmake (2.01a) Sat Sep 22 17:29:49 2012
######################################################################

TEMPLATE        = app
TARGET          = ldforge
SUBDIRS        += ./src

TARGET          = ldforge
DEPENDPATH     += .
INCLUDEPATH    += . ./build/
RC_FILE         = ldforge.rc
RESOURCES       = ldforge.qrc
RCC_DIR         = ./build/
OBJECTS_DIR     = ./build/
MOC_DIR         = ./build/
RCC_DIR         = ./build/
UI_DIR          = ./build/
SOURCES         = src/*.cpp
HEADERS         = src/*.h
FORMS           = ui/*.ui
QT             += opengl network
QMAKE_CXXFLAGS += -std=c++0x
CONFIG         += debug_and_release

CONFIG (debug, debug|release) {
	TARGET = ldforge_debug
} else {
	TARGET = ldforge
}

unix {
	LIBS += -lGLU
}
