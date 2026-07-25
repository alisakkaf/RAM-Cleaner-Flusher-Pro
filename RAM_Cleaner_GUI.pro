# ========================================================================
#          RAM CLEANER & FLUSHER PRO (Qt 5.14.2 C++ GUI)
# ========================================================================
# Created & Developed By: AliSakkaf (By AliSakkaf)
# GitHub:   https://github.com/alisakkaf
# Facebook: https://www.facebook.com/AliSakkaf.Dev/
# Website:  https://alisakkaf.com/
# ========================================================================

QT       += core gui widgets network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++14 strict_c++

TARGET = RAM_Cleaner_Pro
TEMPLATE = app


DEFINES += QT_DEPRECATED_WARNINGS

# ----- Release size optimization flags -----
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CFLAGS_RELEASE   -= -O2
QMAKE_CXXFLAGS_RELEASE += -Os -ffunction-sections -fdata-sections -fmerge-all-constants -fno-stack-protector
QMAKE_CFLAGS_RELEASE   += -Os -ffunction-sections -fdata-sections -fmerge-all-constants -fno-stack-protector
QMAKE_LFLAGS_RELEASE   += -Wl,--gc-sections -s -Wl,--exclude-libs,ALL -Wl,--build-id=none

# Embedding Windows Resource File & Static Linking
win32 {
    RC_FILE = app.rc

    # Windows API System Libraries
    LIBS += -lpsapi -ladvapi32 -luser32 -lshell32 -lole32

    QMAKE_LFLAGS_RELEASE += -static -static-libgcc -static-libstdc++
    QMAKE_LFLAGS_DEBUG   += -static -static-libgcc -static-libstdc++

    # Maximum QRC Compression (zlib level 9)
    QMAKE_RESOURCE_FLAGS += -compress 9 -threshold 0
}


RESOURCES += \
    resources.qrc

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/memory_cleaner.cpp \
    src/app_booster.cpp \
    src/settings_manager.cpp \
    src/icon_provider.cpp \
    src/process_info_dialog.cpp \
    src/optimization_worker.cpp \
    src/process_selection_dialog.cpp \
    src/process_group_dialog.cpp \
    src/update_manager.cpp \
    src/installer_manager.cpp

HEADERS += \
    src/version.h \
    src/win_clean_includes.h \
    src/mainwindow.h \
    src/memory_cleaner.h \
    src/app_booster.h \
    src/settings_manager.h \
    src/icon_provider.h \
    src/process_info_dialog.h \
    src/optimization_worker.h \
    src/process_selection_dialog.h \
    src/process_group_dialog.h \
    src/update_manager.h \
    src/installer_manager.h

FORMS += \
    src/mainwindow.ui

# Output Directories
DESTDIR = $$PWD/bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR = $$PWD/build/moc
UI_DIR = $$PWD/build/ui
