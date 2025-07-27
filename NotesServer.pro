QT       += core gui sql widgets network

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../Notes/FastActions.cpp \
    ../Notes/NetClient.cpp \
    ../Notes/Note.cpp \
    ../Notes/WidgetAlarms.cpp \
    ../Notes/WidgetMain.cpp \
    ../Notes/WidgetNoteEditor.cpp \
    ../include/AdditionalTray.cpp \
    ../include/PlatformDependent.cpp \
    DataBase.cpp \
    NetConstants.cpp \
    Server.cpp \
    WidgetServer.cpp \
    main.cpp

HEADERS += \
    ../Notes/FastActions.h \
    ../Notes/NetClient.h \
    ../Notes/Note.h \
    ../Notes/WidgetAlarms.h \
    ../Notes/WidgetMain.h \
    ../Notes/WidgetNoteEditor.h \
    ../include/AdditionalTray.h \
    ../include/ClickableQWidget.h \
    ../include/MyQTextEdit.h \
    ../include/PlatformDependent.h \
    ../include/TextEditCleaner.h \
    DataBase.h \
    Fields.h \
    NetConstants.h \
    Server.h \
    WidgetServer.h

INCLUDEPATH += \
    C:/Qt/boost_1_72_0 \
    ../include \
	../Notes

DEPENDPATH += \
    C:/Qt/boost_1_72_0 \
    ../include \
	../Notes

LIBS += -lws2_32

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
