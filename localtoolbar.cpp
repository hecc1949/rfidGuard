#include "localtoolbar.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>

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
    QAction* filesAction = new QAction(this);
    filesAction->setIcon(QIcon(QStringLiteral(":/res/Save_32.png")));
    filesAction->setToolTip(tr("文件管理"));
    addAction(filesAction);
    connect(filesAction, &QAction::triggered, [this]() {   emit onLocalToolsAction(1);  });
    addSeparator();

    QAction* netCfgAction = new QAction(this);
    netCfgAction->setIcon(QIcon(QStringLiteral(":/res/wifi.png")));
    netCfgAction->setToolTip(tr("网络配置"));
    addAction(netCfgAction);
    connect(netCfgAction, &QAction::triggered, [this]() {   emit onLocalToolsAction(2);  });
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
}
