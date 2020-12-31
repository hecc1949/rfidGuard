#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QThread>
#include "webpageview.h"
#include "networkchecker.h"
#include "localtoolbar.h"

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

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *ui;

    WebPageView *m_webview;
    QProcess *nodeProc = NULL;

    QThread netmgrThread;

    LocalToolBar *localTools;

    void loadWebView();
    void loadNodejsServer();
public slots:
//    void safeClose();

private slots:
    void doLocalManage(int cmdId);

};

#endif // MAINWINDOW_H
