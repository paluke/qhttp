#ifndef QHTTPCONNECTION_PRIVATE_HPP
#define QHTTPCONNECTION_PRIVATE_HPP
///////////////////////////////////////////////////////////////////////////////

#include "qhttpconnection.hpp"

#include "qhttprequest.hpp"
#include "qhttpresponse.hpp"

#include "private/qhttprequest_private.hpp"
#include "private/qhttpresponse_private.hpp"

#include <QBasicTimer>
///////////////////////////////////////////////////////////////////////////////
namespace qhttp {
namespace server {
///////////////////////////////////////////////////////////////////////////////
class QHttpConnectionPrivate  : public HttpParserBase<QHttpConnectionPrivate>
{
    Q_DECLARE_PUBLIC(QHttpConnection)

public:
    // Since there can only be one request at any time even with pipelining.
    QHttpRequest*           irequest;      ///< latest request
    QHttpResponse*          iresponse;     ///< latest response

    QBasicTimer             itimer;

#   if QHTTP_MESSAGES_LOG > 0
    QByteArray              iinputBuffer;
#   endif

public:
    explicit    QHttpConnectionPrivate(QHttpConnection* q)
        : HttpParserBase(HTTP_REQUEST), q_ptr(q) {
        irequest       = nullptr;
        iresponse      = nullptr;
        isocket        = q;

        QObject::connect(isocket, &QTcpSocket::readyRead, [this](){
            while (isocket->bytesAvailable()) {
                char buffer[4096] = {0};
                size_t readLength = isocket->read(buffer, 4095);

        #       if QHTTP_MESSAGES_LOG > 0
                iinputBuffer.append(buffer);
        #       endif

                parse(buffer, readLength);
            }
        });

        QObject::connect(isocket, &QTcpSocket::disconnected, [this](){
            q_ptr->deleteLater();
        });

        QHTTP_LINE_DEEPLOG
    }

    virtual ~QHttpConnectionPrivate() {
        QHTTP_LINE_DEEPLOG
    }

public:
    int          messageBegin(http_parser *parser);
    int          url(http_parser *parser, const char *at, size_t length);
    int          headerField(http_parser *parser, const char *at, size_t length);
    int          headerValue(http_parser *parser, const char *at, size_t length);
    int          headersComplete(http_parser *parser);
    int          body(http_parser *parser, const char *at, size_t length);
    int          messageComplete(http_parser *parser);

#ifdef USE_CUSTOM_URL_CREATOR
public:
    static QUrl  createUrl(const char *urlData, const http_parser_url &urlInfo);
#endif // USE_CUSTOM_URL_CREATOR

protected:
    QHttpConnection* q_ptr;
};

///////////////////////////////////////////////////////////////////////////////
} // namespace server
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
#endif // QHTTPCONNECTION_PRIVATE_HPP