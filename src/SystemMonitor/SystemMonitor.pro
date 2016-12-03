TEMPLATE = app

CONFIG -= qt
CONFIG(release, debug|release):DEFINES += NDEBUG

QMAKE_LFLAGS += -static
QMAKE_LFLAGS += -nostdlib
contains(QMAKE_HOST.arch, x86_64) {
QMAKE_LFLAGS += -estart
} else {
QMAKE_LFLAGS += -e_start
}
QMAKE_CXXFLAGS += -Wpedantic
QMAKE_CXXFLAGS += -Wzero-as-null-pointer-constant

!contains(QMAKE_HOST.arch, x86_64):LIBS += -lgcc #64bit division

LIBS += -lntdll -lkernel32 -luser32 -lgdi32 -lcomctl32

SOURCES += main.cpp

RC_FILE = res.rc
