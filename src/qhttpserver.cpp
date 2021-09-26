#include <QWebSocket>
#include <QMetaMethod>
#include "private/qhttpserver_private.hpp"

///////////////////////////////////////////////////////////////////////////////
namespace qhttp {
namespace server {
///////////////////////////////////////////////////////////////////////////////

QHttpServer::QHttpServer(QObject *parent)
    : QObject(parent), d_ptr(new QHttpServerPrivate) {
  connect(&d_func()->iwsServer, &QWebSocketServer::newConnection, this, &QHttpServer::forwardWsConnection);
}

QHttpServer::QHttpServer(QHttpServerPrivate &dd, QObject *parent)
    : QObject(parent), d_ptr(&dd) {
}

QHttpServer::~QHttpServer() {
    stopListening();
}

void
QHttpServer::setSslConfig(ssl::Config scnf) {
    d_func()->isslConfig = std::move(scnf);
}

const ssl::Config&
QHttpServer::sslConfig() const {
    return d_func()->isslConfig;
}

bool
QHttpServer::listen(const QHostAddress& address, quint16 port, const ServerHandler& handler) {
    Q_D(QHttpServer);

    d->initialize(this);
    d->ihandler = handler;
    return d->itcpServer->listen(address, port);
}

bool
QHttpServer::isListening() const {
    const Q_D(QHttpServer);

    if ( d->itcpServer )
        return d->itcpServer->isListening();

    return false;
}

void
QHttpServer::stopListening() {
    Q_D(QHttpServer);

    if ( d->itcpServer )
        d->itcpServer->close();

}

quint32
QHttpServer::timeOut() const {
    return d_func()->itimeOut;
}

void
QHttpServer::setTimeOut(quint32 newValue) {
    d_func()->itimeOut = newValue;
}

void
QHttpServer::forwardWsConnection() {
    static const QMetaMethod newWsConnectionSignal = QMetaMethod::fromSignal(&QHttpServer::newWsConnection);
    while (d_func()->iwsServer.hasPendingConnections()) {
        if (isSignalConnected(newWsConnectionSignal))
            emit newWsConnection(d_func()->iwsServer.nextPendingConnection());
        else
            d_func()->iwsServer.nextPendingConnection()->deleteLater();
    }
}

QTcpServer*
QHttpServer::tcpServer() const {
    Q_D(const QHttpServer);
    return d->itcpServer.data();
}

void
QHttpServer::incomingConnection(qintptr handle) {
    Q_D(QHttpServer);

    QHttpConnection* conn = new QHttpConnection(this);
    conn->setSocketDescriptor(handle);
    conn->setTimeOut(d_func()->itimeOut);

    connect(conn, &QHttpConnection::newWebsocketUpgrade,
            this, &QHttpServer::onWebsocketUpgrade);

    emit newConnection(conn);

    if ( d->ihandler )
        QObject::connect(conn, &QHttpConnection::newRequest, d->ihandler);
    else
        incomingConnection(conn);
}

void
QHttpServer::onWebsocketUpgrade(QTcpSocket* socket) {
    socket->setParent(&d_func()->iwsServer);
    d_func()->iwsServer.handleConnection(socket);
    forwardWsConnection();
}

void
QHttpServer::incomingConnection(QHttpConnection *connection) {
    QObject::connect(connection,  &QHttpConnection::newRequest,
                     this,        &QHttpServer::newRequest);
}

///////////////////////////////////////////////////////////////////////////////
} // namespace server
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
