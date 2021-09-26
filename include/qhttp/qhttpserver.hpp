/** HTTP server class.
 * https://github.com/azadkuh/qhttp
 *
 * @author amir zamani
 * @version 2.0.0
 * @date 2014-07-11
  */

#ifndef QHTTPSERVER_HPP
#define QHTTPSERVER_HPP

///////////////////////////////////////////////////////////////////////////////
#include "qhttpfwd.hpp"

#include <QObject>
#include <QHostAddress>

class QWebSocket;
///////////////////////////////////////////////////////////////////////////////
namespace qhttp {
namespace ssl{ struct Config; }
namespace server {
///////////////////////////////////////////////////////////////////////////////

/** The QHttpServer class is a fast, async (non-blocking) HTTP server. */
class QHTTP_API QHttpServer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(quint32 timeOut READ timeOut WRITE setTimeOut)

public:
    /** construct a new HTTP Server. */
    explicit QHttpServer(QObject *parent = nullptr);

    virtual ~QHttpServer();

    /// configure TLS/SSL properties of the server
    void setSslConfig(ssl::Config);
    auto sslConfig() const -> const ssl::Config&;

    /** starts a TCP server on specified address and port.
     * if you provide a server handler, the newRequest() signal won't be
     * emitted.
     *
     * @param address listening address as QHostAddress::Any.  @param port
     * listening port.  @param handler optional server handler (a lambda,
     * std::function, ...) @return false if listening fails.
     */
    bool listen(const QHostAddress& address, quint16 port,
            const ServerHandler& handler = nullptr);

    /** @overload listen() */
    bool listen(quint16 port) {
        return listen(QHostAddress::Any, port);
    }

    /** returns true if server successfully listens. @sa listen() */
    bool isListening() const;

    /** closes the server and stops from listening. */
    void stopListening();

    /** returns timeout value [mSec] for open connections (sockets).
     *  @sa setTimeOut(). */
    quint32 timeOut()const;

    /** set time-out for new open connections in miliseconds [mSec].
     * new incoming connections will be forcefully closed after this time out.
     *  a zero (0) value disables timer for new connections. */
    void setTimeOut(quint32);

signals:
    /** emitted when a client makes a new request to the server if you do not
     * override.
     *  incomingConnection(QHttpConnection *connection);
     * @sa incommingConnection(). */
    void newRequest(QHttpRequest *request, QHttpResponse *response);

    /** emitted when a new connection comes to the server if you do not override
     *  incomingConnection(QHttpConnection *connection);
     * @sa incomingConnection(); */
    void newConnection(QHttpConnection* connection);

    void newWsConnection(QWebSocket* socket);

protected:
    void forwardWsConnection();

    /// returns the tcp server instance if the backend() == ETcpSocket
    QTcpServer* tcpServer() const;

    /** is called when server accepts a new connection.
     * you can override this function for using a thread-pool or ... some other
     * reasons.
     *
     *  the default implementation just connects QHttpConnection::newRequest
     *  signal.
     * @note if you override this method, the signal won't be emitted by
     * QHttpServer.
     * (perhaps, you do not need it anymore).
     *
     * @param connection New incoming connection. */
    virtual void incomingConnection(QHttpConnection* connection);

    /** Transfer the tcp socket to QWebsocketServer **/
    void onWebsocketUpgrade(QTcpSocket* socket);

    /** overrides QTcpServer::incomingConnection() to make a new
     * QHttpConnection.
     * override this function if you like to create your derived
     * QHttpConnection instances.
     *
     * @note if you override this method, incomingConnection(QHttpConnection*)
     * or newRequest(QHttpRequest *, QHttpResponse *) signal won't be called.
     *
     * @see example/benchmark/server.cpp to see how to override.
     */
    virtual void incomingConnection(qintptr handle);

private:
    explicit QHttpServer(QHttpServerPrivate&, QObject *parent);

    Q_DECLARE_PRIVATE(QHttpServer)
    Q_DISABLE_COPY(QHttpServer)
    QScopedPointer<QHttpServerPrivate>  d_ptr;
};

///////////////////////////////////////////////////////////////////////////////
} // namespace server
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
#endif // define QHTTPSERVER_HPP
