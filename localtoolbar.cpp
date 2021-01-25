#include "localtoolbar.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
//#include <QLineEdit>
#include <QFileDialog>
#include <QTreeView>
#include <QMessageBox>
#include "mainwindow.h"
#include "netconfigdialog.h"

LocalToolBar::LocalToolBar(QWidget *parent)
    : QToolBar(parent)
{
    QLayout* layout = this->layout();
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(4);

    setFixedHeight(32);
    setMovable(false);
    toggleViewAction()->setEnabled(false);

    addSeparator();
    fileOutAction = new QAction(this);
    fileOutAction->setIcon(QIcon(QStringLiteral(":/res/Save_32.png")));
    fileOutAction->setToolTip(tr("文件复制到U盘/SD卡"));
    addAction(fileOutAction);
//    connect(fileOutAction, &QAction::triggered, [this]() {   emit onLocalToolsAction(1);  });
    connect(fileOutAction, &QAction::triggered, [this]() {   doFilecopy();  });
    addSeparator();

    netCfgAction = new QAction(this);
    netCfgAction->setIcon(QIcon(QStringLiteral(":/res/wifi.png")));
    netCfgAction->setToolTip(tr("网络配置"));
    addAction(netCfgAction);
    connect(netCfgAction, &QAction::triggered, [this]() {
        NetConfigDialog *netcfgDlg = new NetConfigDialog((QWidget *)(this->parent()));
        netcfgDlg->show();
    });
    addSeparator();

    addSeparator();
    updateFilesAction = new QAction(this);
    updateFilesAction->setIcon(QIcon(QStringLiteral(":/res/format_clear_32.png")));
    updateFilesAction->setToolTip(tr("应用程序升级"));
    addAction(updateFilesAction);
    connect(updateFilesAction, &QAction::triggered, [this]() {   doUpdateFilecopy();  });
    addSeparator();

    //#要把closeButton放最右边，QToolBar的layout中不允许用addStretch()来做拉伸弹簧，但可以加一个带水平扩展
    //SizePolicy的控件来做弹簧。QLabel可以用QWidget代；用QLineEdit则default是水平扩展的，无需setSizePolicy()
    QLabel *_emptyLabel = new QLabel(this);
    _emptyLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    addWidget(_emptyLabel);

    addSeparator();
    QAction *closeAction = new QAction(this);
    closeAction->setIcon(QIcon(QStringLiteral(":/res/close.png")));
    closeAction->setToolTip(tr("关机退出"));
    addAction(closeAction);
    connect(closeAction, &QAction::triggered, [=]() {  parent->close();});
    addSeparator();

    onSysDeviceChange("");
}

void LocalToolBar::doFilecopy()
{
    MainWindow *mainwin = qobject_cast<MainWindow*>(this->parent());
    QString dstPath = "";
    if (QDir("/").exists(mainwin->m_udiskPath))
        dstPath = mainwin->m_udiskPath;
    else if (QDir("/").exists(mainwin->m_tfcardPath))
        dstPath = mainwin->m_tfcardPath;

    if (dstPath.length() ==0)
        return;

    //file dialog
    QString initDir = QCoreApplication::applicationDirPath();
    QStringList filter;
    filter<<"text files(*.txt *.csv *.json)";

    QFileDialog selfile;
    selfile.setDirectory(initDir);
    selfile.setNameFilters(filter);

    selfile.setFileMode(QFileDialog::ExistingFiles);    //按住Ctrl可以多选
    selfile.setAcceptMode(QFileDialog::AcceptOpen);
    selfile.setWindowTitle("选择文件");      //无效
    selfile.setLabelText(QFileDialog::LookIn,"路径");
    selfile.setLabelText(QFileDialog::FileName,"文件名");
    selfile.setLabelText(QFileDialog::FileType,"文件类型");
    selfile.setLabelText(QFileDialog::Accept, "选择");
    selfile.setLabelText(QFileDialog::Reject, "取消");
    selfile.setOptions(QFileDialog::DontUseNativeDialog);   //不用系统Gtk Dialog
    selfile.setViewMode( QFileDialog::Detail);
    //实现多文件选择
    QTreeView *pTreeView = selfile.findChild<QTreeView*>("treeView");
    if (pTreeView)
    {
        pTreeView->setSelectionMode(QAbstractItemView::MultiSelection);
    }
    selfile.setFixedSize(640, 640);

    //执行copy
    int rescnt = 0;
    if (selfile.exec()==QDialog::Accepted)
    {
        QStringList files = selfile.selectedFiles();
        foreach(QString srcfile, files)
        {
            QString dstFile = dstPath+"/"+QFileInfo(srcfile).fileName();
            if (QFile(dstFile).exists())    //copy时有文件QFILE不会覆盖，返回false
            {
                QFile::remove(dstFile);
            }
            if (QFile::copy(srcfile, dstFile))
            {
               rescnt++;
            }
        }
    }
    if (rescnt >0)
    {
#ifndef NODEJS_EMBED_PROC
        system("sync");
#endif
        QMessageBox *messageBox=new QMessageBox(QMessageBox::Information, tr("文件复制"),
                        QString("%1 个文件复制到%2\r\n").arg(rescnt).arg(dstPath), QMessageBox::Close,this);
        QTimer::singleShot(3000, messageBox, SLOT(close()));        //自动关闭messagebox
        messageBox->show();
    }
}

void LocalToolBar::doUpdateFilecopy()
{
    MainWindow *mainwin = qobject_cast<MainWindow*>(this->parent());
    if (!QDir("/").exists(mainwin->m_udiskPath+"/upd"))
    {
        return;
    }
    QMessageBox *messageBox=new QMessageBox(QMessageBox::Information, tr("升级程序文件"),
            tr("确定从U盘复制文件到缓存目录，升级程序文件?"),QMessageBox::Ok | QMessageBox::Cancel, this);
    if (messageBox->exec() == QMessageBox::Ok)
    {
        system("./updfilecopy.sh");
        messageBox=new QMessageBox(QMessageBox::Information, tr("升级程序文件"),
            tr("命令已执行，退出重启后完成升级."), QMessageBox::Close, this);
        messageBox->show();
    }
}

void LocalToolBar::onSysDeviceChange(QString path)
{
//    qDebug()<<"fsChange:"<<path;
    Q_UNUSED(path);

    MainWindow *mainwin = qobject_cast<MainWindow*>(this->parent());
    bool pathOn = (QDir("/").exists(mainwin->m_udiskPath) || QDir("/").exists(mainwin->m_tfcardPath));
    fileOutAction->setEnabled(pathOn);

    pathOn = (QDir("/").exists(mainwin->m_udiskPath+"/upd"));
    updateFilesAction->setEnabled(pathOn);
}
