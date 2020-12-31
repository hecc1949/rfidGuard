#include <QApplication>
#include <QTextCodec>
#include "mainwindow.h"
#include "websocketchannel.h"
#include "devwrapper.h"

int main(int argc, char *argv[])
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));   //locale设置
//    qputenv("QT_IM_MODULE",QByteArray("Qt5Input"));

    QApplication a(argc, argv);

    WebsocketChannel wschannel;    
    MainWindow w;

    DevWrapper devwrapper(&w);
    wschannel.registerObject(QStringLiteral("devwrapper"),&devwrapper);

    w.show();

    return a.exec();
}
