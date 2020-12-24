#ifndef DEVWRAPPER_H
#define DEVWRAPPER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>

class DevWrapper : public QObject
{
    Q_OBJECT
public:
    explicit DevWrapper(QObject *parent = nullptr);
    Q_INVOKABLE QJsonObject doCommand(QString cmd, QJsonArray param);

protected:
//    void closeEvent(QCloseEvent *event);
    void timerEvent(QTimerEvent *);     //使用QObject内置Timer
private:
    int _timerId;
signals:
    void onDeviceEvent(QJsonObject);

public slots:
    void sysclose();

};

#endif // DEVWRAPPER_H
