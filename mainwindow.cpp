#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tinytitlebar.h"
#include <QtWebEngineWidgets>

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
    TinyTitleBar* m_TitleBar = new TinyTitleBar(this);
    setWindowTitle("应用窗口");
    setWindowIcon(QIcon(":res/list_bullets_48px.png"));
//    m_TitleBar->setAutohide(50000);
    ui->statusBar->hide();

    loadWebView();
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addWidget(m_TitleBar);
    layout->addWidget(m_webview);
    centralWidget()->setLayout(layout);
    centralWidget()->setObjectName("workClient");
//    setCentralWidget(m_webview);
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
//#        qDebug()<<"nodeErr:"<<QString(nodeProc->readAllStandardError());
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
