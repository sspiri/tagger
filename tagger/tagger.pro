TEMPLATE = app
CONFIG += console c++17 qt
CONFIG -= app_bundle

QT += core widgets

QMAKE_CXXFLAGS += -std=c++17
LIBS += -ltag -lboost_program_options -lstdc++

SOURCES += \
    main.cpp

HEADERS += \
    file.hpp \
    main_window.hpp \
    tag_editor.hpp
