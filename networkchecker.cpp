#include "networkchecker.h"
#include <QFile>
#include <QMutex>
#include <QDebug>
#include <QEventLoop>
#include <QNetworkInterface>
#include <QThread>

#include "sys/socket.h"
#include "netinet/in.h"
#include "net/if.h"
#include "arpa/inet.h"
#include "sys/ioctl.h"
#include <unistd.h>     //file_id

//#include "linux/sockios.h"
//#include <stdio.h>
//#include <stdint.h>
//#include <fcntl.h>

#define WIFI_SCAN_TIMES  3000
#define MAX_PATH    256

NetworkChecker::NetworkChecker(QObject *parent) : QObject(parent)
{
    //QMetatype注册
    qRegisterMetaType<NetworkInfo_t>("NetworkInfo_t");
    netInfo.netName = "";       //初始，未连接过任何网络
    netInfo.netStatus = 0;

    connect(this, SIGNAL(checkNetworkStatus()), this, SLOT(doCheckEth0()));
    connect(this, SIGNAL(sigCheckDhcp()), this, SLOT(doCheckDhcp()));
    connect(this, SIGNAL(sigCheckWifiStatus()), this, SLOT(doCheckWifiStatus()));
    connect(this, SIGNAL(addWifiAp(QString, QString)), this, SLOT(doAddWifiAP(QString, QString)));
    connect(this, SIGNAL(runNtpDate()), this, SLOT(updateRTC()));

    scanCount = 1024;       //小技巧，避免conncet timeout()时就发出一次tick动作
    scanTimer = new QTimer(this);
    scanTimer->setSingleShot(false);
    connect(scanTimer,SIGNAL(timeout()), this, SLOT(scanTimerTick()));
}

NetworkChecker::~NetworkChecker()
{
    scanTimer->stop();
    if (netInfo.netStatus ==2 ||scanCount <1024)
    {        
        stopWifiLink();     //关闭，注意要有
    }
}

/**
 * @brief NetworkChecker::doCheckEth0 检查有线网连接
 */
void NetworkChecker::doCheckEth0()
{
#ifdef ARM
    char cmdbuf[MAX_PATH];
    char cmdresult[MAX_PATH];   //设置一个合适的长度，以存储每一行输出
    bzero(cmdbuf, MAX_PATH);
    bzero(cmdresult, MAX_PATH);

    bool lineOn = false;
    if (QFile("/sys/class/net/eth0/carrier").exists())
    {
        sprintf(cmdbuf, "cat /sys/class/net/eth0/carrier");
        FILE *pp = popen(cmdbuf, "r");  //建立管道
        fgets(cmdresult, sizeof(cmdresult), pp); //""
        pclose(pp);
        if(strstr(cmdresult, "1"))
            lineOn = true;
    }
    if (!lineOn)
    {
        //断开
        if (netInfo.netName == "eth0")        //连接成功过有线网
        {
            if (netInfo.netStatus !=0)        //断网才发信号，一直没连上不发
            {
                netInfo.netStatus = 0;
                emit sigCheckNetDone(netInfo);
            }
        }
        else
        {
            system("ifdown eth0");      //
            scanCount = 0;
            emit sigCheckWifiStatus();
            scanTimer->start(WIFI_SCAN_TIMES);     //启动wifi检查
        }
    }
    else
    {
        //连上
        netInfo.netName = "eth0";
        if (netInfo.netStatus ==0)
        {
            emit sigCheckDhcp();        //初次连接，检查外网
        }
        else
        {
            emit sigCheckNetDone(netInfo);
        }
    }
#endif
}


/**
 * @brief NetworkChecker::doCheckDhcp 判断是否已获得IP
 */
void NetworkChecker::doCheckDhcp()
{
    int sockfd;
    struct ifreq ifr;
    struct sockaddr_in sin;
    if (netInfo.netName.length()==0)
        return;
    netInfo.Ip = "";

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1)
    {
        qDebug()<<"open socket error";
        return;
    }
    strncpy(ifr.ifr_name, netInfo.netName.toLatin1().data(), IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;
    //ip
    if(ioctl(sockfd, SIOCGIFADDR, &ifr) <0)
        qDebug()<<"socket ioctl error";
    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
    netInfo.Ip = QString(inet_ntoa(sin.sin_addr));

    //mask
    if (ioctl(sockfd, SIOCGIFNETMASK, &ifr) < 0)
        qDebug()<<"socket ioctl error";
    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
    netInfo.mask = QString(inet_ntoa(sin.sin_addr));
    //mac
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0)
        qDebug()<<"socket ioctl error";
    memcpy(&sin, &ifr.ifr_addr, sizeof(sin));
    netInfo.mac = QString(inet_ntoa(sin.sin_addr));
    close(sockfd);

    //gw
    FILE *fp;
    char buf[MAX_PATH];
    char gateway[MAX_PATH];
    bzero(buf, MAX_PATH);
    bzero(gateway, MAX_PATH);
    fp = popen("ip route", "r");
    while(fgets(buf, sizeof(buf), fp) != NULL)
    {
        if(strstr(buf, "default via"))
        {
            sscanf(buf, "%*s%*s%s", gateway);
            break;
        }
    }
    pclose(fp);
    netInfo.gateway = gateway;
    //
    if (netInfo.Ip.length() >0)
    {
        if (netInfo.netName == "eth0")
            netInfo.netStatus = 1;
        else
            netInfo.netStatus = 2;
    }
    emit sigCheckNetDone(netInfo);
}

/**
 * @brief NetworkChecker::scanTimerTick 定时扫描wifi连接状态
 */
void NetworkChecker::scanTimerTick()
{
    scanCount++;
    if (scanCount>=10)
    {
        scanTimer->stop();
    }
    else
    {
        emit sigCheckWifiStatus();
    }
}

void NetworkChecker::doCheckWifiStatus()
{
    QString apName, ipString;
    QString res="";
    FILE *pp = popen("wpa_cli -iwlan0 status", "r"); //建立管道
    if (!pp)
        return;
    char cmdresult[MAX_PATH];
    char key[128];
    char value[128];
    bzero(cmdresult, MAX_PATH);
    bzero(key, 128);
    bzero(value, 128);
    while( fgets(cmdresult, sizeof(cmdresult), pp) != NULL)
    {
        sscanf(cmdresult, "%[^=]=%s", key, value);
        if (QString(key)==QString("ssid"))
            apName = QString(value);
        else if (QString(key)==QString("ip_address"))   //address=>mac
            ipString = QString(value);      //如果udhcpc没有运行，这里是没有IP的
        else if (QString(key)==QString("wpa_state"))
            res = QString(value);
    }
    pclose(pp);
    qDebug()<<"check wifi status:"<<res<<scanCount;

    //#如果wpa_supplicant守护进程没有加载，或者被wpa_cli terminate或killall关闭了，运行wpa_cli出现
    //"Failed to connect to non-global ctrl_ifname: wlan0"错误提示，但是并不在cmdresult返回，返回的
    //是空值。USB网卡未插好或无网卡也是出现这种情况。
    if (strlen(cmdresult)==0 && scanCount==0)
    {
        qDebug()<<"Load wpa_supplicant";
        system("wpa_supplicant -B -Dwext -iwlan0 -c/etc/wpa_supplicant.conf");
        system("udhcpc -iwlan0 -p /var/run/udhcpc.pid -x hostname:$HOSTNAME -q");        //这是需要的
        return;
    }

    //a.COMPLETED; b.SCANNING,ASSOCIATING c.INTERFACE_DISABLED, DISCONNECTED,INACTIVE,4WAY_HANDSHAKE
    if (res == QString("COMPLETED"))
    {
        //wifi已连接成功。但有可能dhcpc未获得IP
        //#这里udhcpc的参数如果不带 -q(Exit after obtaining lease)会导致程序结束时退不到shell，奇怪！
        //用-R(release IP on exit), -b(backgroup)， -n(now, Exit on not obtaining lease)都不行，
        //可能是和/etc/network/interfaces配置的udchp要协调。
        system("udhcpc -iwlan0 -p /var/run/udhcpc.pid -x hostname:$HOSTNAME -q -n");
        //
        scanTimer->stop();
        netInfo.netName = "wlan0";
        if (!netInfo.ssidList.contains(apName))
            netInfo.ssidList.append(apName);
        if (reqSaveConfig || reqUpdate)
        {
            system("wpa_cli -iwlan0 save_config");
            reqSaveConfig = false;
            reqUpdate = false;
        }
        if (scanCount <1024)
        {
            scanCount = 1024;       //停止扫描
            emit sigCheckDhcp();
        }
    }
    else
    {
        if (scanCount>=9)
        {
            //扫描次数到，没有连接上，放弃，进入离线状态
            qDebug()<<"wlan0 disconnect."<<res;
            system("wpa_cli -iwlan0 disc");     //disconncect -reconnect
            netInfo.netName = "";
            netInfo.netStatus = 0;
            emit sigCheckNetDone(netInfo);
            scanTimer->stop();
        }
        else if (scanCount==3)
        {
            //重新找出所有热点，尝试连接
            scanTimer->stop();  //refreshWifi()执行时间可能比Scan间隔时间长，先停Timer避免后续timeout拥挤
            refreshWifi();      //使能连接过但被设置为DISABLED的AP
            scanTimer->start(WIFI_SCAN_TIMES);
        }
        else
        {
            if (res=="DISCONNECTED")    //是断开了连接，可以尝试重连
            {
                system("wpa_cli -iwlan0 reconn");     //disconncect -reconnect
            }
            //偷一点懒：因为addWifiAP()函数是在外线程执行，不能启动scanTimer，在这里代为启动
            if (reqSaveConfig && scanCount==0 && !scanTimer->isActive())
            {
                scanTimer->start(WIFI_SCAN_TIMES);
            }
        }
    }
}

/**
 * @brief NetworkChecker::stopWifiLink 断开wifi连接。退出时有必要执行，否则如果已经连接上，下次上电时
 *  重新dhcpc申请IP会失败，因为路由器还保持着
 */
void NetworkChecker::stopWifiLink()
{    
    system("kill -SIGUSR2 `cat /var/run/udhcpc.pid`");      //释放udhcpc申请的IP
    //几种方式都可以
//#    system("wpa_cli -iwlan0 disc");     //disconncect -reconnect
//#    system("wpa_cli -iwlan0 disable_network 0");  //disable_network 0 -enable_network 0
    system("wpa_cli -iwlan0 terminate");    //terimate wpa_supplicant
//#    system("killall -q wpa_supplicant");

    netInfo.netStatus = 0;
}

/**
 * @brief NetworkChecker::refreshWifi 扫描可用的wifi热点；对连接过但disable的，尝试重新开启它
 * @return
 */
int NetworkChecker::refreshWifi()
{
    char cmdresult[MAX_PATH];
    char bssid[MAX_PATH];
    char frequency[MAX_PATH];
    char signal[MAX_PATH];
    char flag[MAX_PATH];
    char ssid[MAX_PATH];

    //scan扫描现场可用wifi热点
    FILE *pp;
    netInfo.ssidList.clear();
    bool getSsid = false;
    for(int trycnt =0; trycnt<3; trycnt++)
    {
        system("wpa_cli -iwlan0 scan");
        pp = popen("wpa_cli -iwlan0 scan_r", "r"); //建立管道
        if (!pp)
        {
            return(-1);
        }
        bzero(cmdresult, MAX_PATH);
        fgets(cmdresult, sizeof(cmdresult), pp) ; //跳过一行title
        while( fgets(cmdresult, sizeof(cmdresult), pp) != NULL)
        {
            sscanf(cmdresult, "%s\t%s\t%s\t%s\t%s\n", bssid, frequency, signal, flag, ssid);
            if (!netInfo.ssidList.contains(QString::fromLatin1(ssid)))
            {
                netInfo.ssidList.push_back(QString::fromLatin1(ssid));
                getSsid = true;
            }
        }
        pclose(pp);
        if (getSsid)
            break;
        else
        {
            //要一个长延时，否则会Fail_Busy出错， 应该是scan_r之后不能立即scan
            QEventLoop loop2;
            QTimer::singleShot(5000, &loop2, SLOT(quit()));
            loop2.exec();
        }
    }
    if (!getSsid)
    {
        return(-1);
    }

    //list_network在wpa_supplicant.conf中找已经设定(连接过)的ap
    int netid=-1, tmp_netid;
    bzero(cmdresult, MAX_PATH);
    pp = popen("wpa_cli -iwlan0 list_network", "r"); //建立管道
    fgets(cmdresult, sizeof(cmdresult), pp);    //跳过一行
    while(fgets(cmdresult, sizeof(cmdresult), pp) != NULL)
    {
        sscanf(cmdresult, "%d\t%s\t%s\t%s\n", &tmp_netid, ssid, bssid, flag);
        for(int i=0; i<netInfo.ssidList.count(); i++)
        {
            if (netInfo.ssidList.at(i) == QString::fromLatin1(ssid))
            {
                if (QString::fromLatin1(flag).indexOf("DISABLE")>=0)
                {
                    reqUpdate = true;
                }
                netid = tmp_netid;
                break;
            }
        }
    }
    pclose(pp);
    if (netid <0)
        return(-1);
    //重新连接指定net_id的热点，即把它作为首选连接热点
    qDebug()<<"wifi select_network:"<<netid;
    QString cmd;
    cmd = QString("wpa_cli -iwlan0 enable_network %1").arg(netid);
    system(cmd.toLatin1().data());
    cmd = QString("wpa_cli -iwlan0 select_network %1").arg(netid);
    system(cmd.toLatin1().data());
    return(netid);
}

void NetworkChecker::doAddWifiAP(QString ssid, QString psk)
{
#ifdef ARM
    int netid =-1;

    NetworkInfo_t tmpInfo;
    tmpInfo.netStatus = 0;
    tmpInfo.netName = "";
    emit sigCheckNetDone(tmpInfo);    //软断开

    char cmdresult[MAX_PATH];
    QString cmd;
    FILE *pp;
    //先在list_network看wpa_supplicant.conf中是否存在此ssid(只是psk错或被disabled=1而连不上), 有的话
    //就用这个, add_network是无条件添加，save后可能导致wpa_sxx.conf中的network list无限增加
    char t_ssid[MAX_PATH];
    char t_bssid[MAX_PATH];
    char t_flag[MAX_PATH];
    bzero(cmdresult, MAX_PATH);
    int tmp_netid;
    pp = popen("wpa_cli -iwlan0 list_network", "r"); //建立管道
    if (!pp)
        return;
    fgets(cmdresult, sizeof(cmdresult), pp);    //跳过一行
    while(fgets(cmdresult, sizeof(cmdresult), pp) != NULL)
    {
        sscanf(cmdresult, "%d\t%s\t%s\t%s\n", &tmp_netid, t_ssid, t_bssid, t_flag);
        if (ssid == QString::fromLatin1(t_ssid))        //QString(t_ssid)转换出来的是"xxxx"，带多余的双引号
        {
            netid = tmp_netid;
        }
    }
    pclose(pp);

    //
    if (netid<0)
    {
        cmd = QString("wpa_cli -iwlan0 add_network");
        bzero(cmdresult, MAX_PATH);
        pp = popen(cmd.toLatin1().data(), "r"); //建立管道
        if (!pp)
            return;
        while( fgets(cmdresult, sizeof(cmdresult), pp) != NULL)
        {
            sscanf(cmdresult, "%d\n", &netid);
        }
        pclose(pp);
    }
    if (netid<0)
        return;
    //
    cmd = QString("wpa_cli -iwlan0 set_network %1 ssid \'\"%2\"\'").arg(netid).arg(ssid);
    system(cmd.toLatin1().data());
    cmd = QString("wpa_cli -iwlan0 set_network %1 psk \'\"%2\"\'").arg(netid).arg(psk);
    system(cmd.toLatin1().data());

    cmd = QString("wpa_cli -iwlan0 enable_network %1").arg(netid);
    system(cmd.toLatin1().data());

    cmd = QString("wpa_cli -iwlan0 select_network %1").arg(netid);
    system(cmd.toLatin1().data());

    system("wpa_cli -iwlan0 save_config");

    //
    scanCount = 0;
    reqSaveConfig = true;
    netInfo.ssidList.clear();
    emit sigCheckWifiStatus();
#else
    Q_UNUSED(ssid);
    Q_UNUSED(psk);
#endif
}

/**
 * @brief NetworkChecker::updateRTC 网络时钟校准。借用线程
 */
void NetworkChecker::updateRTC()
{
#ifdef ARM
//    qDebug()<<"now update NTP datetime..";
    QString cmd = "ntpdate 0.pool.ntp.org";
    system(cmd.toLatin1());
    cmd = "hwclock -w";
    system(cmd.toLatin1());
#endif
}

