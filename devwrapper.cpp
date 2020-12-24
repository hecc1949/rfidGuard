#include "devwrapper.h"
#include "mainwindow.h"

DevWrapper::DevWrapper(QObject *parent) : QObject(parent)
{
}

void DevWrapper::timerEvent(QTimerEvent *)     //virtual
{
    static int x_data = 0;

    QJsonObject jo;
    jo["event"] = "tick";
    jo["data"] = (++x_data);

    emit(onDeviceEvent(jo));
}

QJsonObject DevWrapper::doCommand(QString cmd, QJsonArray param)
{
    QJsonObject joRes;
    joRes.insert("result", true);
//    joRes["result"] = true;
    if (cmd == "startDev")
    {
        if (param.count() ==0)
            _timerId = startTimer(10000);
        else
            _timerId = startTimer(param[0].toInt());
    }
    return(joRes);
}

void DevWrapper::sysclose()
{
    MainWindow *mainwin = qobject_cast<MainWindow*>(this->parent());
    mainwin->close();
}
