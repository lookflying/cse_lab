TEMPLATE = app
CONFIG += console
CONFIG -= qt

SOURCES += \
    lock_tester.cc \
    lock_smain.cc \
    lock_server.cc \
    lock_demo.cc \
    lock_client.cc \
    gettime.cc \
    rpc/thr_pool.cc \
    rpc/rpctest.cc \
    rpc/rpc.cc \
    rpc/pollmgr.cc \
    rpc/jsl_log.cc \
    rpc/connection.cc

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
    lang/algorithm.h

