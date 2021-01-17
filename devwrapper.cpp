#include "devwrapper.h"
#include "mainwindow.h"
#include <QHostInfo>
#include <QSettings>
//#include <QElapsedTimer>

DevWrapper::DevWrapper(QObject *parent) : QObject(parent),
    inventUHF(this), srcformat(this)
{
    //QMetatype注册
    qRegisterMetaType<IdentifyEPC_t>("IdentifyEPC_t");
    qRegisterMetaType<Membank_data_t>("Membank_data_t");
    qRegisterMetaType<InventifyRecord_t>("InventifyRecord_t");

    inventUHF.srcdatFmt = &srcformat;
    openHwDevices();
    openTagsFinder();
}

DevWrapper::~DevWrapper()
{
    inventUHF.rfidDev = NULL;      //避免invent类重复delete rfidDev
}

void DevWrapper::closeDevices()
{
    if (_devActive)
    {
        if (inventUHF.rfidDev != NULL)
            inventUHF.runInventify(false);
        _devActive = false;
    }
    gpioDeInit();
}

void DevWrapper::timerEvent(QTimerEvent *event)     //virtual
{   
    Q_UNUSED(event);
}

bool DevWrapper::openHwDevices()
{
    QSettings *ini = inventUHF.ini;
    QString port = ini->value("/Device/port").toString();
    int baud = ini->value("/Device/baud").toInt();
    bool ret = false;

    gpioDev = inventUHF.gpioDev;
    gpioInit();
    gpioDev->setGpio(GPIO_ExUart_Power, 1);     //power on
    rfidDev = new Dev_R200(0);
    rfidDev->delay_ms(100);
    gpioDev->setGpio(GPIO_Board_LED3, 1);   //off
    //
    for (int trycnt=0; trycnt<5; trycnt++)
    {
        if (!QFile::exists("/dev/"+port))
        {
            qDebug()<<"serialport not found:"<<port;
            rfidDev->delay_ms(2000);
            QCoreApplication::processEvents();
            continue;
        }
        ret = rfidDev->openCommPort(port, baud);
        if (ret)
        {
            gpioDev->setBuzzle(1);
            break;
        }
    }
    //
    inventUHF.rfidDev = rfidDev;    //!重要
    return(ret);
}

///
/// \brief DevWrapper::openTagsFinder 读卡器软配置和信号-槽连接
/// \return
///
bool DevWrapper::openTagsFinder()
{
    if (rfidDev==NULL || gpioDev==NULL )
        return(false);
    inventUHF.initInterrogator();      //这里要用到dbstore了
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

//====================================================================================

QJsonObject DevWrapper::doCommand(QString cmd, QJsonArray param)
{
    MainWindow *mainwin = qobject_cast<MainWindow*>(this->parent());
    QJsonObject joRes;
    joRes.insert("result", true);
    if (cmd == "runTagFinder")
    {
        bool start = false;
        bool res = false;
        if (param.count()>0)
            start = param[0].toBool();
#ifdef ARM
        int mode = 0;
        if (param.count()>1)
            mode = param[1].toInt();
        res = inventUHF.runInventify(start, mode);     //返回命令是否执行成功
#endif
        if (res)
            _devActive = start;     //转换成是否再扫描状态
        QJsonArray dat;
        QJsonObject jo;
        jo.insert("active", _devActive);
        dat.append(jo);

        joRes["result"] = res;
        joRes.insert("data", dat);
    }
    else if (cmd == "setGpioOut")   {
        if (param.count() >0)
        {
            int sel = param[0].toInt();
            if (sel ==0)
            {
                gpioDev->setGpio(GPIO_AlarmSound, GP_AlarmSound_DIS);
                gpioDev->setGpio(GPIO_AlarmLight, GP_AlarmLigh_DIS);
            }
            else if (sel ==1)
            {
                gpioDev->setGpio(GPIO_AlarmLight, GP_AlarmLigh_EN);
            }
            else if (sel==2)
            {
                gpioDev->setGpio(GPIO_AlarmSound, GP_AlarmSound_EN);
                gpioDev->setGpio(GPIO_AlarmLight, GP_AlarmLigh_EN);
            }
        }
        else
        {
            joRes["result"] = false;
        }
    }
    else if (cmd == "clearInventBuf")
    {
        inventUHF.inventClear(true);
//#        qDebug()<<"Inventify Let Buffer reset..";
    }
    else if (cmd == "ntpUpdate")
    {
        emit mainwin->netChecker->runNtpDate();
    }
    else if (cmd == "sysClose")
    {
//        mainwin->close();
        QTimer::singleShot(100, mainwin, SLOT(close()));        //延时关闭，直接关会有异常
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

void DevWrapper::onInventChangeMode(int mode)
{
    QJsonObject joRes;
    joRes["event"] = "devMsg";
    QJsonArray param;
    QJsonObject jo;
    jo.insert("scanMode", mode);
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
    jo.insert("scanTick", -para);
    param.append(jo);
    joRes["param"] = param;

    emit(onDeviceEvent(joRes));
}

void DevWrapper::onInventUpdate(QByteArray epc, InventifyRecord_t* pRec, bool isNew)
{
    QJsonObject jo;

    jo["id"] = pRec->id;
    jo["epc"] = QString(epc.toHex());
    jo["hitCount"] = pRec->hitCount;

    jo["formatId"] = pRec->formatId;    //
    if (pRec->formatId >=0)
        jo["security"] = pRec->securityBit;
    jo["context"] = QString::fromLatin1(pRec->identifier);

    QJsonObject joRes;
    QJsonArray param;
    if (!isNew)
    {
        joRes["event"] = "tagRepeat";
        param.append(jo);
    }
    else
    {
        jo["title"] ="";            //书名，待扩展

        joRes["event"] = "tagCapture";
        param.append(jo);
    }
    joRes["param"] = param;
    emit(onDeviceEvent(joRes));
}

void DevWrapper::gpioInit()
{
    gpioDev->setGpio(GPIO_AlarmSound, GP_AlarmSound_DIS);
    gpioDev->setGpio(GPIO_AlarmLight, GP_AlarmLigh_DIS);
    gpioDev->setGpio(GPIO_OutRev, 1);       //0有效

    gpioDev->setGpioEdgeTrig(GPIO_AlarmStop, 0);
    gpioDev->setGpioEdgeTrig(GPIO_CheckOut);
    connect(gpioDev, &GpioDev::gpioPinTrig, [this](int gpiono, int val) {
        QJsonObject joRes;
        joRes["event"] = "devMsg";
        QJsonArray param;
        QJsonObject jo;
        if (gpiono == GPIO_AlarmStop)
        {
            if (gpioVal_PassSw != val)
            {
                gpioVal_PassSw = val;
                if (val ==0)        //on
                    jo.insert("AlarmStopReq", 1);
                else
                    jo.insert("AlarmStopReq", 0);
                param.append(jo);
                joRes["param"] = param;
                if (_devActive)
                {
                    emit(onDeviceEvent(joRes));
                }
            }
        }
        else if (gpiono == GPIO_CheckOut)
        {
            if (gpioVal_Checkout != val)
            {
                gpioVal_Checkout = val;
                if (val ==0)        //on
                    jo.insert("CheckoutReq", 1);
                else
                    jo.insert("CheckoutReq", 0);
                param.append(jo);
                joRes["param"] = param;
                if (_devActive)
                {
                    emit(onDeviceEvent(joRes));
                }
            }
        }
//        qDebug()<<"GPIO Trig:"<<gpiono<<val;
    });
}

void DevWrapper::gpioDeInit()
{
    gpioDev->setGpio(GPIO_AlarmSound, GP_AlarmSound_DIS);
    gpioDev->setGpio(GPIO_AlarmLight, GP_AlarmLigh_DIS);
    gpioDev->setGpio(GPIO_OutRev, 1);       //0有效
    disconnect(gpioDev, SIGNAL(gpioPinTrig(int,int)));
}

void DevWrapper::closeServer()
{
    QJsonObject joRes;
    joRes["event"] = "devMsg";
    QJsonArray param;
    QJsonObject jo;
    jo.insert("serverClose", 1);
    param.append(jo);
    joRes["param"] = param;

    emit(onDeviceEvent(joRes));
}
