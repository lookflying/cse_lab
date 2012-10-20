TEMPLATE = app
CONFIG += console
CONFIG -= qt

SOURCES += \
    lock_server.cc \
    lock_client.cc \
    gettime.cc \
    rpc/thr_pool.cc \
    rpc/rpc.cc \
    rpc/pollmgr.cc \
    rpc/jsl_log.cc \
    rpc/connection.cc \
    yfs_client.cc \
    extent_server.cc \
    extent_client.cc \
    fuse.cc

OTHER_FILES += \
    GNUmakefile

HEADERS += \
    lock_server.h \
    lock_protocol.h \
    lock_client.h \
    gettime.h \
    rpc/thr_pool.h \
    rpc/slock.h \
    rpc/rpc.h \
    rpc/pollmgr.h \
    rpc/method_thread.h \
    rpc/marshall.h \
    rpc/jsl_log.h \
    rpc/fifo.h \
    rpc/connection.h \
    lang/verify.h \
    lang/algorithm.h \
    yfs_client.h \
    extent_server.h \
    extent_protocol.h \
    extent_client.h

INCLUDEPATH += rpc lang
LIBS += -lrt -pthread


