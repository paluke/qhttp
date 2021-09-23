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
QHttpServer::listen(const QString &socketOrPort, const ServerHandler &handler) {
    Q_D(QHttpServer);

    bool isNumber   = false;
    quint16 tcpPort = socketOrPort.toUShort(&isNumber);
    if ( isNumber    &&    tcpPort > 0 )
        return listen(QHostAddress::Any, tcpPort, handler);

    d->initialize(ELocalSocket, this);
    d->ihandler = handler;
    return static_cast<QLocalServer*>(d->iserver.data())->listen(socketOrPort);
}

bool
QHttpServer::listen(const QHostAddress& address, quint16 port, const ServerHandler& handler) {
    Q_D(QHttpServer);

    d->initialize(ETcpSocket, this);
    d->ihandler = handler;
    return static_cast<QTcpServer*>(d->iserver.data())->listen(address, port);
}

bool
QHttpServer::isListening() const {
    const Q_D(QHttpServer);

    if (d->iserver ) {
        if ( d->ibackend == ETcpSocket )
            return static_cast<QTcpServer*>(d->iserver.data())->isListening();
        else if (d->ibackend == ELocalSocket)
            return static_cast<QLocalServer*>(d->iserver.data())->isListening();
    }
    return false;
}

void
QHttpServer::stopListening() {
    Q_D(QHttpServer);

    if (d->iserver) {
        if (d->ibackend == ETcpSocket)
            static_cast<QTcpServer*>(d->iserver.data())->close();
        else if (d->ibackend == ELocalSocket) {
            static_cast<QLocalServer*>(d->iserver.data())->close();
            QLocalServer::removeServer(static_cast<QLocalServer*>(d->iserver.data())->fullServerName());
        }
    }
}

quint32
QHttpServer::timeOut() const {
    return d_func()->itimeOut;
}

void
QHttpServer::setTimeOut(quint32 newValue) {
    d_func()->itimeOut = newValue;
}

TBackend
QHttpServer::backendType() const {
    return d_func()->ibackend;
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

    if (d->ibackend == ETcpSocket && d->iserver)
        return qobject_cast<QTcpServer*>(d->iserver.data());

    return nullptr;
}

QLocalServer*
QHttpServer::localServer() const {
    Q_D(const QHttpServer);

    if (d->ibackend == ELocalSocket && d->iserver)
        return qobject_cast<QLocalServer*>(d->iserver.data());

    return nullptr;
}

void
QHttpServer::incomingConnection(qintptr handle) {
    Q_D(QHttpServer);

    QHttpConnection* conn = new QHttpConnection(this);
    conn->setSocketDescriptor(handle, backendType());
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
