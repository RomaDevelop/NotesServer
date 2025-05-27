QT       += core gui sql widgets network

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../Notes/FastActions.cpp \
    ../Notes/NetClient.cpp \
    ../Notes/Note.cpp \
    DataBase.cpp \
    WidgetServer.cpp \
    main.cpp

HEADERS += \
    ../Notes/FastActions.h \
    ../Notes/NetClient.h \
    ../Notes/Note.h \
    DataBase.h \
    Fields.h \
    NetConstants.h \
    WidgetServer.h

INCLUDEPATH += \
    ../include \
	../Notes

DEPENDPATH += \
    ../include \
	../Notes

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
