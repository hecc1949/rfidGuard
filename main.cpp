#include <QApplication>
#include <QTextCodec>
#include "mainwindow.h"
//#include "websocketchannel.h"
//#include "devwrapper.h"
#include <QMutex>

QMutex mutex;

void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    mutex.lock();

    QString text;
    switch(type)
    {
        case QtDebugMsg:
            text = QString("Debug:");
        break;
        case QtWarningMsg:
            text = QString("Warning:");
        break;
        case QtCriticalMsg:
            text = QString("Critical:");
        break;
        case QtInfoMsg:
            text = QString("Info:");
        break;
        case QtFatalMsg:
            text = QString("Fatal:");
        break;
    }

    text.append(QString("[%1]").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
    text.append(QString("%1: Line: %2").arg(QString(context.file)).arg(context.line));
    text.append(QString(" Function: %1").arg(QString(context.function)));

    QString logFileName = QApplication::applicationDirPath() + "/log.txt";
    QFile file(logFileName);
    file.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream text_stream(&file);
    text_stream << text << endl <<QString(" [Message]: %1").arg(msg)<<endl;        //2行
    file.flush();
    file.close();

    //同时在调试窗口输出
//#    std::cout <<text.toStdString().data()<<std::endl<<QString(" [Message]: %1").arg(msg).toStdString().data()<<std::endl;

    mutex.unlock();
}

void logInit()
{
    QString logFileName = QApplication::applicationDirPath() + "/log.txt";
    //以下这段代码的含义是初始化时检查日志文件是否存在一个月之前的日志，如果存在删除之
    if (QFile(logFileName).exists())
    {
        QMutexLocker locker(&mutex);        //mutex.lock();

        QFile file(logFileName);
        file.open(QIODevice::ReadOnly);
        QTextStream textStream(&file);
        QString temp;
        QStringList tempList;
        QRegExp regExp(".*(20\\d\\d-\\d\\d-\\d\\d).*");
        while ((temp = textStream.readLine()).isEmpty() == false)
        {
            if(temp.indexOf(regExp) >= 0)
            {
                QDate date = QDate::fromString(regExp.cap(1), "yyyy-MM-dd");
                QDate currentDate = QDate::currentDate();
                //判断当前行所记载的日期和现今的日期天数之差是否大于记录该条日志时的那个月的天数
                if(date.daysTo(currentDate) < date.day())
                {
                    tempList << temp;
                    tempList << textStream.readLine();      //2行
                }
            }
        }
        file.close();
        file.open(QIODevice::Truncate | QIODevice::WriteOnly);
        textStream.setDevice(&file);
        for(auto iterator = tempList.begin(); iterator != tempList.end(); iterator++)
        {
            textStream << *iterator << endl;
        }
        file.close();
        //mutex.unlock();
    }

    //注册MessageHandler
    qInstallMessageHandler(outputMessage);
}


int main(int argc, char *argv[])
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));   //locale设置
//    qputenv("QT_IM_MODULE",QByteArray("Qt5Input"));

    QApplication a(argc, argv);

    QStringList arguments = QCoreApplication::arguments();
    if (arguments.indexOf("-log")>0)
    {
        logInit();
    }

//    WebsocketChannel wschannel;
    MainWindow w;
//    QDir::setCurrent(QCoreApplication::applicationDirPath());
//    DevWrapper devwrapper(&w);
//    wschannel.registerObject(QStringLiteral("devwrapper"),&devwrapper);

    w.show();
/*    int res = a.exec();
    wschannel.deregisterObject(&devwrapper);
    return res;
*/
    return a.exec();
}
