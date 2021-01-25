#ifndef DEVWRAPPER_H
#define DEVWRAPPER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include "srcdatformat.h"
#include "inventproxy.h"
#include "../urfidLib/inc/gpiodev.h"

/*
#define GPIO_LED_CameraVideo    (32 + 30)       //GPIOB30
#define GPIO_Relay              (32 + 31)       //GPIOB31
#define GPIO_Buzzle             (4*32 + 4)      //GPIOE4
#define GPIO_LED_0              (4*32 + 6)      //GPIOE6

#define GPIO_Board_LED2         (2*32+17)       //GPIOC17   识别时闪烁
#define GPIO_Board_LED3         (2*32+24)       //GPIOC24   保留
#define GPIO_ExUart_Power       (28)            //GPIOA28   模块用5V电源开关
*/

//ouput
#define GPIO_AlarmLight     (3*32 + 29)         //GPIOD29, J11.5~6
#define GPIO_AlarmSound     (3*32 + 28)         //GPIOD28, J11.7~8
#define GPIO_WorkStatus     GPIO_Relay          //GPIOB31

//input
#define GPIO_AlarmStop      (3*32 + 30)         //GPIOD30, J11.1~2
#define GPIO_CheckOut       (3*32 + 31)         //GPIOD31, J11.3~4

#define GPIO_ModeIn         (2*32 + 5)          //GPIOC5, JP2.8,不隔离
//--
#define GP_AlarmSound_EN    0       //0有效
//#define GP_AlarmSound_DIS   1
#define GP_AlarmLight_EN     0
//#define GP_AlarmLight_DIS    1

#define GP_WorkStatus_EN    1
//#define GP_WorkStatus_DIS   0

typedef struct {
    int gpioNo;
    int value_en;
    int direct;
}   GpioPinDef_t;

class DevWrapper : public QObject
{
    Q_OBJECT
public:
    explicit DevWrapper(QObject *parent = nullptr);
    ~DevWrapper();
    Q_INVOKABLE QJsonObject doCommand(QString cmd, QJsonArray param);

    void closeServer();
protected:
    void timerEvent(QTimerEvent *);     //使用QObject内置Timer
private:
    int _timerId;
    QJsonArray getSysInfo();

    InventProxy inventUHF;
    SrcdatFormat srcformat;
    RfidReaderMod *rfidDev = NULL;
    bool _devActive = false;
    GpioDev *gpioDev = NULL;

    int gpioVal_PassSw;
    int gpioVal_Checkout;
    QList<GpioPinDef_t> gpioOuts;

    bool openHwDevices();
    bool openTagsFinder();
    void gpioInit();
    void gpioDeInit();

private slots:
    void dev_cmdResponse(QString info, int cmd, int status);
    void dev_errMsg(QString info, int errCode);
    void dev_readMemBank(Membank_data_t bankdat, int info);

    void onInventTickAck(int para);
    void onInventChangeMode(int mode);
    void onInventUpdate(QByteArray epc, InventifyRecord_t *pRec, bool isNew);

signals:
    void onDeviceEvent(QJsonObject);

public slots:
    void closeDevices();

};

#endif // DEVWRAPPER_H
