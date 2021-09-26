/** private imeplementation.
 * https://github.com/azadkuh/qhttp
 *
 * @author amir zamani
 * @version 2.0.0
 * @date 2014-07-11
  */

#ifndef QHTTPSERVER_CONNECTION_PRIVATE_HPP
#define QHTTPSERVER_CONNECTION_PRIVATE_HPP
///////////////////////////////////////////////////////////////////////////////

#include "httpparser.hxx"
#include "qhttp/qhttpserverconnection.hpp"
#include "qhttp/qhttpserverrequest.hpp"
#include "qhttp/qhttpserverresponse.hpp"
#include "qhttp/qhttpserver.hpp"
#include "qhttp/qhttpsslsocket.hpp"

#include "private/qhttpserverrequest_private.hpp"
#include "private/qhttpserverresponse_private.hpp"

#include <QBasicTimer>
#include <QFile>
///////////////////////////////////////////////////////////////////////////////
namespace qhttp {
namespace server {
///////////////////////////////////////////////////////////////////////////////
class QHttpConnectionPrivate  :
    public details::HttpRequestParser<QHttpConnectionPrivate>
{
    Q_DECLARE_PUBLIC(QHttpConnection)

public:
 explicit QHttpConnectionPrivate(QHttpConnection* q, QHttpServer* server)
     : q_ptr(q), iserver(server) {

     QObject::connect(q_func(), &QHttpConnection::disconnected,
                      [this]() { release(); });
 }

 virtual ~QHttpConnectionPrivate() = default;

 void createSocket(qintptr sokDesc) {

     initTcpSocket(sokDesc);

    }

    void release() {
        // if socket drops and http_parser can not call
        // messageComplete, dispatch the ilastRequest
        finalizeConnection();

        isocket.disconnectAllQtConnections();
        isocket.release();

        if ( ilastRequest ) {
            ilastRequest->deleteLater();
            ilastRequest  = nullptr;
        }

        if ( ilastResponse ) {
            ilastResponse->deleteLater();
            ilastResponse = nullptr;
        }

        q_func()->deleteLater();
    }

public:
    void onReadyRead() {
        while ( isocket.bytesAvailable() > 0 ) {
            QByteArray buffer(isocket.readRaw());
            parse(buffer.constData(), buffer.size());
            if (iparser.http_errno != 0) {
                QHttpResponse response(q_ptr);
                response.setStatusCode(qhttp::ESTATUS_BAD_REQUEST);
                response.addHeader("connection", "close");
                response.end("<h1>400 Bad Request</h1>\n");
                release(); // release the socket if parsing failed
                return;
            }
            if (iparser.upgrade)
                return;
        }
    }

    void finalizeConnection() {
        if ( ilastRequest == nullptr )
            return;

        ilastRequest->d_func()->finalizeSending([this]{
            emit ilastRequest->end();
        });
    }

public:
    int  messageBegin(http_parser* parser);
    int  url(http_parser* parser, const char* at, size_t length);
    int  status(http_parser*, const char*, size_t) {
        return 0;   // not used in parsing incoming request.
    }
    int  headerField(http_parser* parser, const char* at, size_t length);
    int  headerValue(http_parser* parser, const char* at, size_t length);
    int  headersComplete(http_parser* parser);
    int  body(http_parser* parser, const char* at, size_t length);
    int  messageComplete(http_parser* parser);

private:
    void initTcpSocket(qintptr sokDesc) {
        const auto& sslConfig = iserver->sslConfig();
        if (sslConfig.hasLocal()) {
            auto sslsok = new ssl::Socket(q_func());
            sslsok->setup(sslConfig);
            sslsok->setSocketDescriptor(sokDesc);
            sslsok->setPeerVerifyMode(QSslSocket::VerifyNone);
            sslsok->startServerEncryption();
            isocket.itcpSocket = sslsok;

        } else {
            auto sok = new QTcpSocket(q_func());
            sok->setSocketDescriptor(sokDesc);
            isocket.itcpSocket = sok;
        }

        QObject::connect(isocket.itcpSocket, &QTcpSocket::readyRead, [this]() {
            onReadyRead();
        });
        QObject::connect(
            isocket.itcpSocket, &QTcpSocket::bytesWritten, [this]() {
                auto btw = isocket.itcpSocket->bytesToWrite();
                if (btw == 0 && ilastResponse)
                    emit ilastResponse->allBytesWritten();
        });
        QObject::connect(
            isocket.itcpSocket,
            &QTcpSocket::disconnected,
            q_func(),
            &QHttpConnection::disconnected,
            Qt::QueuedConnection);
    }

protected:
    QHttpConnection* const q_ptr;
    QHttpServer* const     iserver;

    QByteArray             itempUrl;

    // Since there can only be one request/response pair per connection at any
    // time even with pipelining.
    QHttpRequest*          ilastRequest  = nullptr;
    QHttpResponse*         ilastResponse = nullptr;

    ServerHandler          ihandler      = nullptr;

};

///////////////////////////////////////////////////////////////////////////////
} // namespace server
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
#endif // QHTTPSERVER_CONNECTION_PRIVATE_HPP
