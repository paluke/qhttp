#include "private/qhttpserverconnection_private.hpp"

///////////////////////////////////////////////////////////////////////////////
namespace qhttp {
namespace server {
///////////////////////////////////////////////////////////////////////////////
QHttpConnection::QHttpConnection(QHttpServer *parent)
    : QObject(parent), d_ptr(new QHttpConnectionPrivate(this, parent)) { }

QHttpConnection::QHttpConnection(QHttpConnectionPrivate& dd, QHttpServer* parent)
    : QObject(parent), d_ptr(&dd) { }

void
QHttpConnection::setSocketDescriptor(qintptr sokDescriptor, TBackend backendType) {
    d_ptr->createSocket(sokDescriptor, backendType);
}

QHttpConnection::~QHttpConnection() = default;

void
QHttpConnection::setTimeOut(quint32 miliSeconds) {
    if ( miliSeconds != 0 ) {
        d_func()->itimeOut = miliSeconds;
        d_func()->itimer.start(miliSeconds, Qt::CoarseTimer, this);
    }
}

void
QHttpConnection::killConnection() {
    d_func()->isocket.close();
}

TBackend
QHttpConnection::backendType() const {
    return d_func()->isocket.ibackendType;
}

QTcpSocket*
QHttpConnection::tcpSocket() const {
    return d_func()->isocket.itcpSocket;
}

QLocalSocket*
QHttpConnection::localSocket() const {
    return d_func()->isocket.ilocalSocket;
}

void
QHttpConnection::onHandler(const ServerHandler &handler) {
    d_func()->ihandler = handler;
}

void
QHttpConnection::timerEvent(QTimerEvent *) {
    killConnection();
}

///////////////////////////////////////////////////////////////////////////////

// if user closes the connection, ends the response or by any other reason the
// socket disconnects, then the irequest and iresponse instances may have
// been deleted. In these situations reading more http body or emitting end()
// for incoming request are not possible:
// if ( ilastRequest == nullptr )
//     return 0;


int
QHttpConnectionPrivate::messageBegin(http_parser*) {
    itempUrl.clear();
    itempUrl.reserve(128);

    if ( ilastRequest )
        ilastRequest->deleteLater();

    ilastRequest = new QHttpRequest(q_func());
    return 0;
}

int
QHttpConnectionPrivate::url(http_parser*, const char* at, size_t length) {
    Q_ASSERT(ilastRequest);

    itempUrl.append(at, static_cast<int>(length));
    return 0;
}

int
QHttpConnectionPrivate::headerField(http_parser*, const char* at, size_t length) {
    if ( ilastRequest == nullptr )
        return 0;

    // insert the header we parsed previously
    // into the header map
    if ( !itempHeaderField.isEmpty() && !itempHeaderValue.isEmpty() ) {
        // header names are always lower-cased
        ilastRequest->d_func()->iheaders.insert(
                    itempHeaderField.toLower(),
                    itempHeaderValue
                    );
        // clear header value. this sets up a nice
        // feedback loop where the next time
        // HeaderValue is called, it can simply append
        itempHeaderField.clear();
        itempHeaderValue.clear();
    }

    itempHeaderField.append(at, static_cast<int>(length));
    return 0;
}

int
QHttpConnectionPrivate::headerValue(http_parser*, const char* at, size_t length) {
    if ( ilastRequest == nullptr )
        return 0;

    itempHeaderValue.append(at, static_cast<int>(length));
    return 0;
}

int
QHttpConnectionPrivate::headersComplete(http_parser* parser) {
    if ( ilastRequest == nullptr )
        return 0;

    ilastRequest->d_func()->iurl = QUrl(itempUrl);

    // set method
    ilastRequest->d_func()->imethod =
            static_cast<THttpMethod>(parser->method);

    // set version
    ilastRequest->d_func()->iversion = QString("%1.%2")
                                       .arg(parser->http_major)
                                       .arg(parser->http_minor);

    // Insert last remaining header
    ilastRequest->d_func()->iheaders.insert(
                itempHeaderField.toLower(),
                itempHeaderValue
                );

    itempHeaderField.clear();
    itempHeaderValue.clear();

    // set client information
    if ( isocket.ibackendType == ETcpSocket ) {
        ilastRequest->d_func()->iremoteAddress = isocket.itcpSocket->peerAddress().toString();
        ilastRequest->d_func()->iremotePort    = isocket.itcpSocket->peerPort();

    } else if ( isocket.ibackendType == ELocalSocket ) {
        ilastRequest->d_func()->iremoteAddress = isocket.ilocalSocket->fullServerName();
        ilastRequest->d_func()->iremotePort    = 0; // not used in local sockets
    }

    if ( isocket.ibackendType == ETcpSocket && parser->upgrade ) {
        // QWebsocketServer will handle the tcp socket from there
        // Using QWebSocketServer::handleConnection

        // We don't need the request anymore
        QObject::disconnect(ilastRequest, nullptr, nullptr, nullptr);

        // Disconnect socket to avoid triggering readyRead()
        isocket.disconnectAllQtConnections();

        // We need to rollback data to pass to QWebSocketServer.
        isocket.rollbackTransaction();

        // Remove parenting and references
        isocket.itcpSocket->setParent(nullptr);

        emit q_ptr->newWebsocketUpgrade(isocket.itcpSocket);

        isocket.itcpSocket = nullptr;
        release();

        return 2;
    }

    if ( ilastResponse )
        ilastResponse->deleteLater();

    // Validate data, we won't need to rollback.
    isocket.commitTransaction();
    ilastResponse = new QHttpResponse(q_func());

    ilastResponse->d_func()->ikeepAlive = ::http_should_keep_alive(parser);

    // close the connection if response was the last packet
    QObject::connect(ilastResponse, &QHttpResponse::done, q_ptr, [this](bool wasTheLastPacket){
        ikeepAlive = !wasTheLastPacket;
        if ( wasTheLastPacket ) {
            isocket.flush();
            isocket.close();
        }
    });

    // we are good to go!
    if ( ihandler )
        ihandler(ilastRequest, ilastResponse);
    else
        emit q_ptr->newRequest(ilastRequest, ilastResponse);

    return 0;
}

int
QHttpConnectionPrivate::body(http_parser*, const char* at, size_t length) {
    if ( ilastRequest == nullptr )
        return 0;

    ilastRequest->d_func()->ireadState = QHttpRequestPrivate::EPartial;

    if ( ilastRequest->d_func()->icollectRequired ) {
        if ( !ilastRequest->d_func()->append(at, length) ) {
            // forcefully dispatch the ilastRequest
            finalizeConnection();
        }

        return 0;
    }

    emit ilastRequest->data(QByteArray(at, static_cast<int>(length)));
    return 0;
}

int
QHttpConnectionPrivate::messageComplete(http_parser*) {
    if ( ilastRequest == nullptr )
        return 0;

    // request is done
    finalizeConnection();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
} // namespace server
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
