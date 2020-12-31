#ifndef DEVWRAPPER_H
#define DEVWRAPPER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include "srcdatformat.h"
#include "inventproxy.h"

class DevWrapper : public QObject
{
    Q_OBJECT
public:
    explicit DevWrapper(QObject *parent = nullptr);
    Q_INVOKABLE QJsonObject doCommand(QString cmd, QJsonArray param);
//    Q_INVOKABLE QJsonArray getSysInfo();

protected:
//    void closeEvent(QCloseEvent *event);
    void timerEvent(QTimerEvent *);     //使用QObject内置Timer
private:
    int _timerId;
    QJsonArray getSysInfo();

    InventProxy inventUHF;
    RfidReaderMod *rfidDev = NULL;
    SrcdatFormat srcformat;
    bool openTagsFinder();

private slots:
    void dev_cmdResponse(QString info, int cmd, int status);
    void dev_errMsg(QString info, int errCode);
    void dev_readMemBank(Membank_data_t bankdat, int info);

//    void onReadTagPoolUpdate(IdentifyEPC_t tagEpc, int infoSerial);
//    void onReadTagTick(int count, int updId);

    void onInventTickAck(int para);
    void onInventChangeMode(int mode);
    void onInventUpdate(QByteArray epc, InventifyRecord_t *pRec, bool isNew);

signals:
    void onDeviceEvent(QJsonObject);

public slots:
//    void sysclose();

};

#endif // DEVWRAPPER_H
