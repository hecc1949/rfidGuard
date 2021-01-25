#include <QtWebEngineWidgets>
#include "mainwindow.h"
#include "ui_mainwindow.h"
//#include "tinytitlebar.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), wschannel(), devwrapper(this)
{    
    wschannel.registerObject(QStringLiteral("devwrapper"),&devwrapper);

    ui->setupUi(this);
#ifdef ARM
    QDesktopWidget* desktop = QApplication::desktop();
    setGeometry(0, 0, desktop->width(),desktop->height());
#else
    setGeometry(0, 0, 1280, 720);
#endif
    setWindowTitle("应用窗口");
    setWindowIcon(QIcon(":res/list_bullets_48px.png"));
    ui->statusBar->hide();

//    localTools = new LocalToolBar(this);
//    addToolBar(localTools);
//    connect(localTools, SIGNAL(onLocalToolsAction(int)), this, SLOT(doLocalManage(int)));

    loadNodejsServer();

    loadWebView();
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
//    layout->addWidget(m_TitleBar);
    layout->addWidget(m_webview);
    centralWidget()->setLayout(layout);
    centralWidget()->setObjectName("workClient");
//    setCentralWidget(m_webview);

    //网络检测
    m_netInfo.netStatus = 0;
    netChecker = new NetworkChecker(NULL);          //手工delete
    connect(netChecker, &NetworkChecker::sigCheckNetDone, this, [=](NetworkInfo_t netInfo)    {
        m_netInfo = netInfo;
    });
    netChecker->moveToThread(&netmgrThread);
    netmgrThread.start();

    QSettings ini(QCoreApplication::applicationDirPath()+"/config.ini",QSettings::IniFormat);
    m_tfcardPath = ini.value("/Operator/TFCardPath").toString();
    if (m_tfcardPath.length()==0)
        m_tfcardPath = "/media/hcsd";       //如果是空值，Dir()类会在qDebug()输出message, 没必要
    m_udiskPath = ini.value("/Operator/UDiskPath").toString();
    if (m_udiskPath.length()==0)
        m_udiskPath = "/media/udisk";

    localTools = new LocalToolBar(this);
    addToolBar(localTools);

    fsWatcher.addPath("/media");
    connect(&fsWatcher, SIGNAL(directoryChanged(QString)), localTools, SLOT(onSysDeviceChange(QString)));

    QTimer *startAppTimer = new QTimer(this);
    startAppTimer->setSingleShot(true);
    connect(startAppTimer, &QTimer::timeout, this,[=]() {
        netChecker->checkNetworkStatus();
    });
    startAppTimer->start(2000);

//    qDebug() <<"curent timeZone:"<< QDateTime::currentDateTime().timeZone().id(); //Asia/Shanghai

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *)
{
    QElapsedTimer rtimer;
    rtimer.start();

//    Q_UNUSED(event);
//    m_webview->close();
//    m_webview->load(QUrl(""));
    setStyleSheet("background-color: #008080");     //
    repaint();

#ifdef ARM
    devwrapper.closeDevices();
#endif

/*    if (nodeProc != NULL)
    {
        nodeProc->terminate();
        nodeProc = NULL;
    }
    */

#ifdef NODEJS_EMBED_PROC
    netChecker->keepWifiOnClose = (nodeProc != NULL);
#endif

    while(netmgrThread.isRunning())
    {
        netmgrThread.quit();
        netmgrThread.wait();
    }
    delete netChecker;

    devwrapper.closeServer();


#ifdef ARM
#ifndef NODEJS_EMBED_PROC
    if (QDir("/").exists(m_tfcardPath))
    {
        //卸载TF卡
//        system("sync");
        system((QString("umount -l %1").arg(m_tfcardPath)).toLatin1().data());
    }
    if (QDir("/").exists(m_udiskPath))
    {
        //卸载U盘
//        system("sync");
        system((QString("umount -f %1").arg(m_udiskPath)).toLatin1().data()); //
    }
#endif
#endif


    while(rtimer.elapsed()<100)
    {
        QCoreApplication::processEvents();
    }
}

#if 0
void MainWindow::safeClose()
{
#ifdef ARM
    devwrapper.closeDevices();
#endif

//    m_webview->load(QUrl(""));
    setStyleSheet("background-color: #008080");     //
    repaint();
//    wschannel.closeChannel();

    if (nodeProc != NULL)
    {
        nodeProc->terminate();
        nodeProc = NULL;
    }
    while(netmgrThread.isRunning())
    {
        netmgrThread.quit();
        netmgrThread.wait();
    }
    delete netChecker;

#ifdef ARM
    QString tfcardPath = "/media/hcsd";
    if (QDir("/").exists(tfcardPath))
    {
        //卸载TF卡
        system("sync");
        system((QString("umount -l %1").arg(tfcardPath)).toLatin1().data());
    }
    QString udiskPath = "/media/udisk";
    if (QDir("/").exists(udiskPath))
    {
        //卸载U盘
        system("sync");
        system((QString("umount -f %1").arg(udiskPath)).toLatin1().data()); //
    }
#endif

//    qDebug()<<"program safe close..";
/*
    QTimer *closeTimer = new QTimer(this);
    closeTimer->setSingleShot(true);
//    connect(closeTimer, SIGNAL(timeout()), this, SLOT(close()));
    connect(closeTimer, &QTimer::timeout, [this]()  {
        qDebug()<<"program safe close..";
        close();
    });
    closeTimer->start(100);
*/
    this->close();
}
#endif

void MainWindow::loadWebView()
{
    m_webview = new WebPageView(this);
    WebPage *page = new WebPage(QWebEngineProfile::defaultProfile(), m_webview);
    m_webview->setPage(page);
//    m_webview->setUrl(QUrl(QStringLiteral("http://baidu.com")));
//    m_webview->setUrl(QUrl(QStringLiteral("http://www.sina.com.cn")));
//    m_webview->setUrl(QUrl(QStringLiteral("http://news.163.com")));
//    m_webview->load(QUrl("file://"+QCoreApplication::applicationDirPath() + "/tvideo.html"));

    QUrl url = QUrl(QStringLiteral("http://localhost:2280/wod.html"));
    m_webview->setUrl(url);

    connect(m_webview, &QWebEngineView::urlChanged,[this](const QUrl &url) {
        ui->statusBar->showMessage(url.toString());
    });
    connect(m_webview, &WebPageView::loadProgressStatus, [=](int progress) {
        if (progress<0)     //服务端未运行，加载失败，重新连接
        {
            loadNodejsServer();
            //delay
            QEventLoop loop;
            QTimer::singleShot(2500, &loop, SLOT(quit()));
            loop.exec(QEventLoop::ExcludeUserInputEvents);
            m_webview->setUrl(url);
        }
    });
    connect(m_webview, &WebPageView::naviActionChanged, [this](QWebEnginePage::WebAction, bool) {
    });
    connect(m_webview, &WebPageView::linkHovered, [this](const QUrl &url)   {
        ui->statusBar->showMessage(url.toString());
    });
    connect(m_webview, &WebPageView::onDownloading, [this](QString prompt)    {
        ui->statusBar->showMessage(prompt, 0);
    });
}

void MainWindow::loadNodejsServer()
{
    if (nodeProc != NULL)
        return;
    nodeProc = new QProcess();
    nodeProc->setProcessChannelMode(QProcess::SeparateChannels);   //stdout和stderr分开

    connect(nodeProc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
          [=](int exitCode, QProcess::ExitStatus exitStatus)    {
        Q_UNUSED(exitCode);
        Q_UNUSED(exitStatus);
        nodeProc = NULL;
    });
    connect(nodeProc, &QProcess::readyReadStandardOutput, this, [=]()   {
        while(nodeProc->canReadLine())
        {
            QByteArray buffer(nodeProc->readLine());
            qDebug()<<"nodejs:"<<QString(buffer);
        }
    });
    connect(nodeProc, &QProcess::readyReadStandardError, this, [=]()   {
        qDebug()<<"nodeErr:"<<QString(nodeProc->readAllStandardError());
    });


    QString workdir = QCoreApplication::applicationDirPath();
    workdir = workdir.left(workdir.indexOf('-'));       //去掉proj-build-pc的后段
    nodeProc->setWorkingDirectory(workdir);
    QStringList args;       //注意，args中String不能包含空格！
    args <<"wod-index.js";
//    args << (workdir + "/wod-index.js");

#ifdef NODEJS_EMBED_PROC
    nodeProc->start(tr("node"), args);
    if (!nodeProc->waitForStarted(100))
    {
        QMessageBox::warning(this,tr("warning"), "QProcess启动nodejs失败");
        nodeProc = NULL;
    }
    qDebug()<<"run nodejs in Qt-process..";
#else
    if (!nodeProc->startDetached(tr("node"), args))     //Detach方式不能用waitForStarted()判断启动成功
    {
        QMessageBox::warning(this,tr("warning"), "QProcess启动nodejs失败");
        nodeProc = NULL;
    }
    qDebug()<<"run nodejs detached.";
#endif
}

void MainWindow::doLocalManage(int cmdId)
{
//    Q_UNUSED(cmdId);
    qDebug()<<"Local cmd:"<<cmdId;
}
