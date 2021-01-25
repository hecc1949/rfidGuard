#include "websocketchannel.h"
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QWebSocket>

WebSocketTransport::WebSocketTransport(QWebSocket *socket)
    : QWebChannelAbstractTransport(socket), m_socket(socket)
{
    connect(socket, &QWebSocket::textMessageReceived,
            this, &WebSocketTransport::textMessageReceived);
    connect(socket, &QWebSocket::disconnected, this, &WebSocketTransport::deleteLater);
}

WebSocketTransport::~WebSocketTransport()
{
    m_socket->deleteLater();
}

void WebSocketTransport::sendMessage(const QJsonObject &message)
{
    QJsonDocument doc(message);
    m_socket->sendTextMessage(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

void WebSocketTransport::textMessageReceived(const QString &messageData)
{
    QJsonParseError error;
    QJsonDocument message = QJsonDocument::fromJson(messageData.toUtf8(), &error);
    if (error.error) {
        qWarning() << "Failed to parse text message as JSON object:" << messageData
                   << "Error is:" << error.errorString();
        return;
    } else if (!message.isObject()) {
        qWarning() << "Received JSON message that is not an object: " << messageData;
        return;
    }
    emit messageReceived(message.object(), this);
}

//================================================================================================

WebsocketChannel::WebsocketChannel(QObject *parent) : QObject(parent)
{
    m_server = new QWebSocketServer(QStringLiteral("QWebChannel Server"), QWebSocketServer::NonSecureMode);
    if (!m_server->listen(QHostAddress::AnyIPv4, 2285))
    {
        qFatal("Failed to open web socket server.");
        QCoreApplication::exit(1);
    }
    m_channel = new QWebChannel();
    connect(m_server, &QWebSocketServer::newConnection,[this]() {
        m_channel->connectTo(new WebSocketTransport(m_server->nextPendingConnection()));
    });
}

