/** @file qsocket.hpp
 *
 * @copyright (C) 2016
 * @date 2016.05.26
 * @version 1.0.0
 * @author amir zamani <azadkuh@live.com>
 *
 */

#ifndef __QHTTP_SOCKET_HPP__
#define __QHTTP_SOCKET_HPP__

#include "qhttp/qhttpheaders.hpp"

#include <QTcpSocket>
#include <QLocalSocket>
#include <QUrl>
///////////////////////////////////////////////////////////////////////////////
namespace qhttp {
namespace details {
///////////////////////////////////////////////////////////////////////////////

/** an adapter for different socket types.
 * the main purpose of QHttp was to create a small HTTP server with ability to
 * support UNIX sockets (QLocalSocket)
 */
class QSocket
{
public:
    void close() {
        if ( igenericSocket ) {
            if (ibackendType == ETcpSocket)
                static_cast<QTcpSocket*>(igenericSocket)->disconnectFromHost();
            igenericSocket->close();
        }
    }

    void release() {
        close();
        if ( igenericSocket )
            igenericSocket->deleteLater();

        igenericSocket   = nullptr;
    }

    void flush() {
        if ( igenericSocket ) {
            if ( ibackendType == ETcpSocket )
                static_cast<QTcpSocket*>(igenericSocket)->flush();
            else if ( ibackendType == ELocalSocket )
                static_cast<QLocalSocket*>(igenericSocket)->flush();
        }
    }

    bool isOpen() const {
        if ( igenericSocket && igenericSocket->isOpen() ) {
            if ( ibackendType == ETcpSocket )
                return static_cast<QTcpSocket*>(igenericSocket)->state() == QTcpSocket::ConnectedState;
            else if ( ibackendType == ELocalSocket )
                return static_cast<QLocalSocket*>(igenericSocket)->state() == QLocalSocket::ConnectedState;
        }
        return false;
    }

    void connectTo(const QUrl& url) {
        if ( igenericSocket && ibackendType == ELocalSocket )
            static_cast<QLocalSocket*>(igenericSocket)->connectToServer(url.path());
    }

    void connectTo(const QString& host, quint16 port) {
        if ( igenericSocket && ibackendType == ETcpSocket )
            static_cast<QTcpSocket*>(igenericSocket)->connectToHost(host, port);
    }

    qint64 readRaw(char* buffer, qint64 maxlen) {
        startTransaction();
        if ( igenericSocket )
            return igenericSocket->read(buffer, maxlen);
        return 0;
    }

    QByteArray readRaw() {
        startTransaction();
        if ( igenericSocket )
            return igenericSocket->readAll();
        return QByteArray();
    }

    void writeRaw(const QByteArray& data) {
        if ( igenericSocket )
            igenericSocket->write(data);
    }

    qint64 bytesAvailable() {
        if ( igenericSocket )
            return igenericSocket->bytesAvailable();

        return 0;
    }

    void disconnectAllQtConnections() {
        if ( igenericSocket )
            QObject::disconnect(igenericSocket, 0, 0, 0);
    }

    void startTransaction() {
        if ( igenericSocket && !igenericSocket->isTransactionStarted() )
            igenericSocket->startTransaction();
    }

    void rollbackTransaction() {
        if ( igenericSocket ) igenericSocket->rollbackTransaction();
    }

    void commitTransaction() {
        if ( igenericSocket ) igenericSocket->commitTransaction();
    }

public:
    TBackend     ibackendType = ETcpSocket;
    QIODevice* igenericSocket = nullptr;
}; // class QSocket

///////////////////////////////////////////////////////////////////////////////
} // namespace details
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
#endif // __QHTTP_SOCKET_HPP__
