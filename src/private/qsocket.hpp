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
        if ( itcpSocket ) {
            itcpSocket->disconnectFromHost();
            itcpSocket->close();
        }
    }

    void release() {
        close();
        if ( itcpSocket )
            itcpSocket->deleteLater();

        itcpSocket   = nullptr;
    }

    void flush() {
        if ( itcpSocket )
            itcpSocket->flush();
    }

    bool isOpen() const {
        if ( itcpSocket )
            return itcpSocket->isOpen()
                && itcpSocket->state() == QTcpSocket::ConnectedState;

        return false;
    }

    void connectTo(const QString& host, quint16 port) {
        if ( itcpSocket )
            itcpSocket->connectToHost(host, port);
    }

    qint64 readRaw(char* buffer, qint64 maxlen) {
        startTransaction();
        if ( itcpSocket ) {
            return itcpSocket->read(buffer, maxlen);
        }
        return 0;
    }

    QByteArray readRaw() {
        startTransaction();
        if ( itcpSocket) {
            return itcpSocket->readAll();
        }
        return QByteArray();
    }

    void writeRaw(const QByteArray& data) {
        if ( itcpSocket )
            itcpSocket->write(data);
    }

    qint64 bytesAvailable() {
        if ( itcpSocket )
            return itcpSocket->bytesAvailable();
        return 0;
    }

    void disconnectAllQtConnections() {
        if ( itcpSocket )
            QObject::disconnect(itcpSocket, 0, 0, 0);
    }

    void startTransaction() {
        if (itcpSocket && !itcpSocket->isTransactionStarted())
            itcpSocket->startTransaction();
    }

    void rollbackTransaction() {
        if (itcpSocket) itcpSocket->rollbackTransaction();
    }

    void commitTransaction() {
        if (itcpSocket) itcpSocket->commitTransaction();
    }

public:
    QTcpSocket*       itcpSocket   = nullptr;
}; // class QSocket

///////////////////////////////////////////////////////////////////////////////
} // namespace details
} // namespace qhttp
///////////////////////////////////////////////////////////////////////////////
#endif // __QHTTP_SOCKET_HPP__
