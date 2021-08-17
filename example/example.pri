equals(ENABLE_QHTTP_CLIENT, "1") {
    DEFINES *= QHTTP_HAS_CLIENT
}

!equals(STATIC_LIB, "1") {
    DEFINES *= QHTTP_DYNAMIC_LIB
}

CONFIG      *= c++14 c++11

INCLUDEPATH *= $$PWD/../include
win32 {
    CONFIG(release, debug|release): LIBS *= -L$$OUT_PWD/../../src/release -lqhttp
    CONFIG(debug, debug|release):   LIBS *= -L$$OUT_PWD/../../src/debug -lqhttp
} else {
    LIBS *= -L$$OUT_PWD/../../src -lqhttp
}
