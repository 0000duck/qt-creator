TEMPLATE = lib
CONFIG+=dll
TARGET = QmlJSDebugger

unix:QMAKE_CXXFLAGS_DEBUG += -O3

include(../../../../src/qtcreatorlibrary.pri)
include(qmljsdebugger-lib.pri)
