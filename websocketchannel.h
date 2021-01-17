#ifndef WEBSOCKETCHANNEL_H
#define WEBSOCKETCHANNEL_H

#include <QObject>
#include <QWebChannel>
#include <QWebSocketServer>
#include <QWebChannelAbstractTransport>

//子类
class WebSocketTransport : public QWebChannelAbstractTransport
{
    Q_OBJECT
public:
    explicit WebSocketTransport(QWebSocket *socket);
    virtual ~WebSocketTransport();

    void sendMessage(const QJsonObject &message) override;

private slots:
    void textMessageReceived(const QString &message);

private:
    QWebSocket *m_socket;
};

class WebsocketChannel : public QObject
{
    Q_OBJECT
public:
    explicit WebsocketChannel(QObject *parent = nullptr);
//    ~WebsocketChannel();

    void registerObject(QString objname, QObject *obj)  {
//        wrapperObj = obj;
//        qDebug()<<"register Object:"<<objname;
        m_channel->registerObject(objname, obj);    //要手动定义object名，这个名在js中作为object名用。传入的obj.objname为空值
    }
/*
    void deregisterObject(QObject *obj) {
        m_channel->deregisterObject(obj);
    }
*/
/*
    void closeChannel() {
        m_server->close();
    }
*/
private:
    QWebSocketServer *m_server;
    QWebChannel *m_channel;
//    QObject *wrapperObj;

signals:
    void clientConnected(WebSocketTransport *client);

public slots:
};

#endif // WEBSOCKETCHANNEL_H
