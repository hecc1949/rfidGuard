#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QThread>
#include "websocketchannel.h"
#include "devwrapper.h"
#include "webpageview.h"
#include "networkchecker.h"
#include "localtoolbar.h"
#include <QFileSystemWatcher>

//编译配置
#ifndef ARM
#define     NODEJS_EMBED_PROC
#endif

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    NetworkInfo_t m_netInfo;
    NetworkChecker *netChecker;

    QString m_tfcardPath;
    QString m_udiskPath;
    void closeWebPage() {
        m_webview->close();
    }
protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *ui;

    //Registered new object after initialization, existing clients won't be notified!
    WebsocketChannel wschannel;
    DevWrapper devwrapper;

    WebPageView *m_webview;
    QProcess *nodeProc = NULL;

    QThread netmgrThread;

    LocalToolBar *localTools;
    QFileSystemWatcher fsWatcher;

    void loadWebView();
    void loadNodejsServer();
public slots:
//    void safeClose();

private slots:
    void doLocalManage(int cmdId);

};

#endif // MAINWINDOW_H
