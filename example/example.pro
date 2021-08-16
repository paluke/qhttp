TEMPLATE = subdirs

include(example.pri)

SUBDIRS += helloworld
SUBDIRS += postcollector
SUBDIRS += basic-server
SUBDIRS += ssl-simple

equals(ENABLE_QHTTP_CLIENT, "1") {
    SUBDIRS += keep-alive
}

