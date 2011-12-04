#-------------------------------------------------
#
# Project created by QtCreator 2011-11-15T21:12:05
#
#-------------------------------------------------

QT       += core gui

TARGET = IRCBoilerplate
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    session.cpp \
    connection.cpp \


HEADERS  += mainwindow.h \
    session.h \
    connection.h \


FORMS    += mainwindow.ui


win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../communi/lib/ -lCommuni
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../communi/lib/ -lCommunid
else:symbian: LIBS += -lCommuni
else:unix: LIBS += -L$$PWD/../communi/lib/ -lCommuni

INCLUDEPATH += $$PWD/../communi/include
DEPENDPATH += $$PWD/../communi/include
DEFINES += COMMUNI_SHARED

# Create a macro from the Git describe command, to use in about boxes.
BUILDSTR = '\\"$$system(git describe)\\"'
DEFINES += BUILD=\"$${BUILDSTR}\"






















