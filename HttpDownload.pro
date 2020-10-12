#-------------------------------------------------
#
# Project created by QtCreator 2013-10-09T17:41:35
#
#-------------------------------------------------

QT       += core gui
QT       += network
DEFINES  += QT_NO_SSL

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = HttpDownload
TEMPLATE = app


SOURCES += main.cpp\
        httpdownload.cpp

HEADERS  += httpdownload.h

FORMS    += httpdownload.ui
