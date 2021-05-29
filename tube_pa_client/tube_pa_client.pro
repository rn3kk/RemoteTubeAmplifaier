QT += gui widgets network

CONFIG += c++11 console
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
 ../common/ampinfo.cpp \
 ../common/frontmodel.cpp \
 ../common/jsonprotokol.cpp \
 ../common/model.cpp \
 ../common/socketwrapper.cpp \
 ampitemwidget.cpp \
 form.cpp \
 main.cpp \
 mainwindow.cpp \
 broadcastreceiver.cpp \
 mechpanel.cpp \
 vfo.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

FORMS += \
 ampitemwidget.ui \
 form.ui \
 mainwindow.ui \
 mechpanel.ui \
 vfo.ui

HEADERS += \
 ../common/ampinfo.h \
 ../common/frontmodel.h \
 ../common/jsonprotokol.h \
 ../common/model.h \
 ../common/socketwrapper.h \
 ampitemwidget.h \
 form.h \
 mainwindow.h \
 broadcastreceiver.h \
 mechpanel.h \
 vfo.h
