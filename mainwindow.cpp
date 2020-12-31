#include <QtWebEngineWidgets>
#include "mainwindow.h"
#include "ui_mainwindow.h"
//#include "tinytitlebar.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{    
    ui->setupUi(this);
#ifdef ARM
    QDesktopWidget* desktop = QApplication::desktop();
    setGeometry(0, 0, desktop->width(),desktop->height());
#else
    setGeometry(0, 0, 1280, 720);
#endif
//    TinyTitleBar* m_TitleBar = new TinyTitleBar(this);
//    m_TitleBar->hide();

    setWindowTitle("应用窗口");
    setWindowIcon(QIcon(":res/list_bullets_48px.png"));
//    m_TitleBar->setAutohide(50000);
    ui->statusBar->hide();


    localTools = new LocalToolBar(this);
    addToolBar(localTools);
    connect(localTools, SIGNAL(onLocalToolsAction(int)), this, SLOT(doLocalManage(int)));

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

    QTimer *startAppTimer = new QTimer(this);
    startAppTimer->setSingleShot(true);
    connect(startAppTimer, &QTimer::timeout, this,[=]() {
        netChecker->checkNetworkStatus();
    });
    startAppTimer->start(2000);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    setStyleSheet("background-color: #008080");     //
    repaint();

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

    if (nodeProc != NULL)
    {
        nodeProc->terminate();
        nodeProc = NULL;
    }

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

}

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
//        naviToolBar->setUrlLine(url.toString());
        ui->statusBar->showMessage(url.toString());
    });
    connect(m_webview, &WebPageView::loadProgressStatus, [=](int progress) {
        if (progress<0)     //服务端未运行，加载失败，重新连接
        {
            if (nodeProc == NULL)
            {
                loadNodejsServer();
            }
            //delay
            QEventLoop loop;
            QTimer::singleShot(1000, &loop, SLOT(quit()));
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
    qDebug()<<"load nodejs...";
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
    nodeProc->start(tr("node"), args);
//    nodeProc->startDetached(tr("node"), args);

    if (!nodeProc->waitForStarted(500))
    {
        QMessageBox::warning(this,tr("warning"), "nodejs启动失败");
        nodeProc = NULL;
    }

}

void MainWindow::doLocalManage(int cmdId)
{
    Q_UNUSED(cmdId);
}
