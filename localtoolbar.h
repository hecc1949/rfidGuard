#ifndef LOCALTOOLBAR_H
#define LOCALTOOLBAR_H

#include <QObject>
#include <QToolBar>

class LocalToolBar : public QToolBar
{
    Q_OBJECT
public:
    explicit LocalToolBar(QWidget *parent = nullptr);

signals:
    void onLocalToolsAction(int index);

};

#endif // LOCALTOOLBAR_H
