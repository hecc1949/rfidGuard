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

signals:
//    void onLocalToolsAction(int index);

private slots:
    void doFilecopy();

};

#endif // LOCALTOOLBAR_H
