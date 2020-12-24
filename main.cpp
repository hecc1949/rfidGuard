#include <QApplication>
#include "mainwindow.h"
#include "websocketchannel.h"
#include "devwrapper.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    WebsocketChannel wschannel;    
    MainWindow w;

    DevWrapper devwrapper(&w);
    wschannel.registerObject(QStringLiteral("devwrapper"),&devwrapper);

    w.show();

    return a.exec();
}
