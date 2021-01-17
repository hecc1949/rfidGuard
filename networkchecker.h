#ifndef NETWORKCHECKER_H
#define NETWORKCHECKER_H

#include <QObject>
#include <QTimer>

typedef struct  {
    int netStatus;          //0: no linked, 1-eth0 linked, 2-wifi wlan0 linked
    QString netName;
    QString Ip;

    QString mask;
    QString mac;
    QString gateway;
    QStringList ssidList;

}   NetworkInfo_t;

class NetworkChecker : public QObject
{
    Q_OBJECT
public:
    explicit NetworkChecker(QObject *parent = nullptr);
    ~NetworkChecker();
    bool keepWifiOnClose = false;

private:
    QTimer *scanTimer;
    int scanCount;
    bool reqSaveConfig = false;
    int reqUpdate = false;

    NetworkInfo_t netInfo;

    void stopWifiLink();
//    int refreshWifi();
signals:
    //private
    void sigCheckDhcp();
    void sigCheckWifiStatus();
    //public
    void checkNetworkStatus();
    void addWifiAp(QString ssid, QString psk);
    void runNtpDate();
    //out
    void sigCheckNetDone(NetworkInfo_t info);    

private slots:
    void doCheckEth0();
    void doCheckWifiStatus();
    void doCheckDhcp();
    void scanTimerTick();

    void doAddWifiAP(QString ssid, QString psk);
    void updateRTC();

public slots:
    int refreshWifi();
};

#endif // NETWORKCHECKER_H
