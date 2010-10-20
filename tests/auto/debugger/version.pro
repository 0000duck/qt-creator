CONFIG += qtestlib testcase
QT -= gui

UTILSDIR    = ../../../src/libs

DEBUGGERDIR = ../../../src/plugins/debugger

INCLUDEPATH += $$DEBUGGERDIR $$UTILSDIR

SOURCES += \
    tst_version.cpp \
    $$DEBUGGERDIR/gdb/gdbmi.cpp \

