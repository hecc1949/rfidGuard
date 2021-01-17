#include "netconfigdialog.h"
#include "ui_netconfigdialog.h"
#include "mainwindow.h"

NetConfigDialog::NetConfigDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NetConfigDialog)
{
    ui->setupUi(this);

    ui->btnWifiScan->setIcon(style()->standardPixmap(QStyle::SP_BrowserReload));
    ui->btnOk->setIcon(style()->standardPixmap(QStyle::SP_DialogOkButton));

    ui->btnCancel->setIcon(style()->standardPixmap(QStyle::SP_DialogCloseButton));
    connect(ui->btnCancel, &QPushButton::clicked, [this](){ this->reject(); });

    ui->cmWifiSsid->clear();
    MainWindow *mainwin = qobject_cast<MainWindow*>(this->parent());
    for(int i=0; i<mainwin->m_netInfo.ssidList.length(); i++)
    {
        ui->cmWifiSsid->addItem(mainwin->m_netInfo.ssidList[i]);
    }
    //
    connect(mainwin->netChecker, &NetworkChecker::sigCheckNetDone, this, [=](NetworkInfo_t netInfo)    {
        ui->cmWifiSsid->clear();
        for(int k=0; k<netInfo.ssidList.length(); k++)  {
            ui->cmWifiSsid->addItem(netInfo.ssidList[k]);
        }
    });

}

NetConfigDialog::~NetConfigDialog()
{
    delete ui;
}

void NetConfigDialog::on_btnWifiScan_clicked()
{
    MainWindow *mainwin = qobject_cast<MainWindow*>(this->parent());
//    mainwin->netChecker->sigCheckWifiStatus();
    mainwin->netChecker->refreshWifi();

    ui->btnWifiScan->setEnabled(false);
    QTimer *timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this,[=]() {   ui->btnWifiScan->setEnabled(true);  });
    timer->start(2000);

}

void NetConfigDialog::on_btnOk_clicked()
{
    if (ui->cmWifiSsid->currentText().length()<1 || ui->edWifiPsk->text().length()<1)
    {
        ui->cmWifiSsid->setFocus();
        return;
    }

    MainWindow *mainwin = qobject_cast<MainWindow*>(this->parent());
    mainwin->netChecker->addWifiAp(ui->cmWifiSsid->currentText(), ui->edWifiPsk->text());

    QEventLoop loop;
    QTimer::singleShot(1000, &loop, SLOT(quit()));
    loop.exec(QEventLoop::ExcludeUserInputEvents);

    this->close();
}
