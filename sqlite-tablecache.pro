######################################################################
# Automatically generated by qmake (3.1) Sat May 25 13:37:39 2019
######################################################################

TEMPLATE = app
QT -= gui
QT += sql
TARGET = sqlite-tablecache
INCLUDEPATH += .
HEADERS += DbScope.h TableCache.h CarCache.h
SOURCES += main.cpp DbScope.cpp CarCache.cpp
QMAKE_LFLAGS += -lsqlite3
