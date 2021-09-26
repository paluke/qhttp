/** private imeplementation.
 * https://github.com/azadkuh/qhttp
 *
 * @author amir zamani
 * @version 2.0.0
 * @date 2014-07-11
  */

#ifndef QHTTPCLIENT_REQUEST_PRIVATE_HPP
#define QHTTPCLIENT_REQUEST_PRIVATE_HPP
///////////////////////////////////////////////////////////////////////////////
#include "httpwriter.hxx"
#include "qhttp/qhttpclient.hpp"
#include "qhttp/qhttpclientrequest.hpp"

#include <QTcpSocket>

///////////////////////////////////////////////////////////////////////////////
namespace qhttp {
namespace client {
///////////////////////////////////////////////////////////////////////////////
class QHttpRequestPrivate :
    public details::HttpWriter<details::HttpRequestBase, QHttpRequestPrivate>
{
    Q_DECLARE_PUBLIC(QHttpRequest)

public:
    explicit    QHttpRequestPrivate(QHttpClient* cli, QHttpRequest* q)
        : q_ptr(q), iclient(cli) { }

    virtual    ~QHttpRequestPrivate() = default;

    void        initialize() {
        iversion    = "1.1";

        isocket.itcpSocket    = iclient->tcpSocket();
    }

    QByteArray  makeTitle();

    void        prepareHeadersToWrite();

protected:
    QHttpRequest* const  q_ptr;
    QHttpClient* const   iclient;
};

///////////////////////////////////////////////////////////////////////////////
} // namespace client
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
#endif // QHTTPCLIENT_REQUEST_PRIVATE_HPP
