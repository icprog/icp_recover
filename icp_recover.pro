#-------------------------------------------------
#
# Project created by QtCreator 2015-09-08T11:21:48
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = icp_recover
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    src_libmodbus/modbus-tcp.c \
    src_libmodbus/modbus.c \
    src_libmodbus/modbus-data.c \
    src_libmodbus/modbus-rtu.c

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

LIBS += -lws2_32

#add ico to windows application
RC_FILE = myapp.rc

RESOURCES += \
    res.qrc
