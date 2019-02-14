TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle qt

QMAKE_CXXFLAGS += -std=c++17
LIBS += -ltag -lboost_program_options -lstdc++

SOURCES += \
		main.cpp
