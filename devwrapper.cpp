#include "devwrapper.h"
#include "mainwindow.h"
#include <QHostInfo>

DevWrapper::DevWrapper(QObject *parent) : QObject(parent),
    inventUHF(this)
{
    //QMetatype注册
    qRegisterMetaType<IdentifyEPC_t>("IdentifyEPC_t");
    qRegisterMetaType<Membank_data_t>("Membank_data_t");
    qRegisterMetaType<InventifyRecord_t>("InventifyRecord_t");
}

void DevWrapper::timerEvent(QTimerEvent *)     //virtual
{
/*    static int x_data = 0;

    QJsonObject jo;
    jo["event"] = "tick";
    jo["data"] = (++x_data);

    emit(onDeviceEvent(jo));
    */
}

QJsonObject DevWrapper::doCommand(QString cmd, QJsonArray param)
{
    MainWindow *mainwin = qobject_cast<MainWindow*>(this->parent());
    QJsonObject joRes;
    joRes.insert("result", true);
//    joRes["result"] = true;
    if (cmd == "startDev")
    {
        if (param.count() ==0)
            _timerId = startTimer(10000);
        else
            _timerId = startTimer(param[0].toInt());
    }
    else if (cmd == "ntpUpdate")
    {
        emit mainwin->netChecker->runNtpDate();
    }
    else if (cmd == "sysClose")
    {
        mainwin->close();
    }
    else if (cmd == "getSysInfo")
    {
        QJsonArray dat = getSysInfo();
        joRes.insert("data", dat);
    }
    return(joRes);
}


QJsonArray DevWrapper::getSysInfo()
{  
    QJsonArray joRes;
    QJsonObject jRow;
    MainWindow *mainwin = qobject_cast<MainWindow*>(this->parent());

    jRow["name"] = "主机名";
    jRow["value"] = QHostInfo::localHostName();
    joRes.append(jRow);
    jRow["name"] = "版本号";
    jRow["value"] = VERSION_STRING;
    joRes.append(jRow);

    jRow["name"] = "netStatus";
    jRow["value"] = mainwin->m_netInfo.netStatus;
    joRes.append(jRow);
    jRow["name"] = "IP";
    jRow["value"] = mainwin->m_netInfo.Ip;
//    jRow["value"] = "127.0.0.1";
    joRes.append(jRow);

    return(joRes);
}

//====================================================================================
bool DevWrapper::openTagsFinder()
{
    inventUHF.initInterrogator();      //这里要用到dbstore了
    rfidDev = inventUHF.rfidDev;

    connect(rfidDev, SIGNAL(cmdResponse(QString, int, int)), this, SLOT(dev_cmdResponse(QString, int, int)), Qt::QueuedConnection);
    connect(rfidDev, SIGNAL(errorResponse(QString, int)), this, SLOT(dev_errMsg(QString, int)), Qt::QueuedConnection);
    connect(rfidDev, SIGNAL(readMemBank(Membank_data_t, int)), this, SLOT(dev_readMemBank(Membank_data_t, int)), Qt::QueuedConnection);

    connect(rfidDev, SIGNAL(tagIdentify(IdentifyEPC_t,int)), &inventUHF, SLOT(onTagIdentified(IdentifyEPC_t,int)),Qt::QueuedConnection);

    connect(&inventUHF, SIGNAL(ScanTickAck(int)), this, SLOT(onInventTickAck(int)), Qt::QueuedConnection);
    connect(&inventUHF, SIGNAL(ChangeScanMode(int)), this, SLOT(onInventChangeMode(int)), Qt::QueuedConnection);
    connect(&inventUHF, SIGNAL(InventUpdate(QByteArray, InventifyRecord_t*, bool)),
            this, SLOT(onInventUpdate(QByteArray, InventifyRecord_t*, bool)), Qt::QueuedConnection);

    return(true);
}

void DevWrapper::dev_cmdResponse(QString info, int cmd, int status)      //slot
{
    QString s1 = rfidDev->packDumpInfo(0, info, cmd, status);
    if (s1 == info)
    {
        qDebug()<<info.toLatin1();
    }
}
void DevWrapper::dev_errMsg(QString info, int errCode)       //slot
{
    QString s1 = rfidDev->packDumpInfo(1, info, errCode, 0);
    if (s1 ==info)
    {
        qDebug()<<info;
    }
}
void DevWrapper::dev_readMemBank(Membank_data_t bankdat, int info)   //slot
{
    Q_UNUSED(bankdat);
    Q_UNUSED(info);
}

void DevWrapper::onInventChangeMode(int mode)
{
    QJsonObject joRes;
    joRes["event"] = "devMsg";
    QJsonArray param;
    QJsonObject jo;
    joRes.insert("scanMode", mode);
    param.append(jo);
    joRes["param"] = param;

    emit(onDeviceEvent(joRes));
}

void DevWrapper::onInventTickAck(int para)
{
    QJsonObject joRes;
    joRes["event"] = "devMsg";
    QJsonArray param;
    QJsonObject jo;
    joRes.insert("scanTick", para);
    param.append(jo);
    joRes["param"] = param;

    emit(onDeviceEvent(joRes));
}

void DevWrapper::onInventUpdate(QByteArray epc, InventifyRecord_t* pRec, bool isNew)
{
/*    QJsonObject jo;
    QString s1;
    jo["append"] = isNew;
    jo["id"] = pRec->id;
    s1 = epc.toHex();
    jo["epc"] = s1;
    jo["hitCount"] = pRec->hitCount;
    if (pRec->hitTimeElapsed<1000)
    {
        s1 = QString(".%1").arg(pRec->hitTimeElapsed/10);
    }
    else
    {
        int r1 = (pRec->hitTimeElapsed % 1000)/100;
        s1 = QString("%1.%2").arg(pRec->hitTimeElapsed/1000).arg(r1);
    }
    jo["hitTime"] = s1;
    jo["formatId"] = pRec->formatId;    //
    jo["groupId"] = pRec->groupIdA;
    jo["context"] = QString::fromLatin1(pRec->identifier);
    if (!isNew)
    {
        emit(inventTagUpdate(jo));
        return;
    }

    //
    jo["title"] ="";            //书名，待扩展
    if (pRec->formatId >=0)
        jo["securityBit"] = QString::number(pRec->securityBit);
    emit(inventTagUpdate(jo));
    */

}
