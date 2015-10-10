TEMPLATE = app

CONFIG -= app_bundle
CONFIG -= qt
CONFIG(release, debug|release):DEFINES += NDEBUG

QMAKE_LFLAGS += -static

LIBS += -lgdi32 -lcomdlg32

SOURCES += main.cpp

RC_FILE = res.rc
