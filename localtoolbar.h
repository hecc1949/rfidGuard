#ifndef LOCALTOOLBAR_H
#define LOCALTOOLBAR_H

#include <QObject>
#include <QToolBar>

class LocalToolBar : public QToolBar
{
    Q_OBJECT
public:
    explicit LocalToolBar(QWidget *parent = nullptr);

private:
    QAction* fileOutAction;
    QAction* netCfgAction;
    QAction* updateFilesAction;
signals:
//    void onLocalToolsAction(int index);

private slots:
    void doFilecopy();
    void doUpdateFilecopy();
public slots:
    void onSysDeviceChange(QString path);
};

#endif // LOCALTOOLBAR_H
