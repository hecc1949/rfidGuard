#ifndef NETCONFIGDIALOG_H
#define NETCONFIGDIALOG_H

#include <QDialog>

namespace Ui {
class NetConfigDialog;
}

class NetConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NetConfigDialog(QWidget *parent = 0);
    ~NetConfigDialog();

private slots:
    void on_btnOk_clicked();

    void on_btnWifiScan_clicked();

private:
    Ui::NetConfigDialog *ui;
};

#endif // NETCONFIGDIALOG_H
