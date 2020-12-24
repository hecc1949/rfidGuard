#include "tinytitlebar.h"
#include <QStyle>

#define _TITLEBAR_SIZE  28

TinyTitleBar::TinyTitleBar(QWidget *parent) : QWidget(parent)
{    
    setFixedHeight(_TITLEBAR_SIZE);
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setProperty("titleBar", true);
    setObjectName("titleBar");

    setAutoFillBackground(true);        // 不继承父组件的背景色
    setBackgroundRole(QPalette::Highlight);     //要用这个来设置背景色，setStyleSheet只能设置到Label背景色

    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(_TITLEBAR_SIZE-4, _TITLEBAR_SIZE-4);
    m_iconLabel->setScaledContents(true);
    QPixmap pixTitle = style()->standardPixmap(QStyle::SP_ComputerIcon);    //使用系统内置图标
    m_iconLabel->setPixmap(pixTitle);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_closeButton = new QPushButton(this);  //QToolButton比QPushButton复杂，不好做透明效果
    m_closeButton->setFixedSize(_TITLEBAR_SIZE, _TITLEBAR_SIZE);
    m_closeButton->setObjectName("closeButton");
    m_closeButton->setIconSize(QSize(_TITLEBAR_SIZE-4, _TITLEBAR_SIZE-4));
//#    QPixmap pix = style()->standardPixmap(QStyle::SP_DialogCloseButton);    //SP_TitleBarCloseButton;
//    m_closeButton->setIcon(pix);
    m_closeButton->setIcon(QPixmap(QPixmap(":res/close_24.png")));
    m_closeButton->setFocusPolicy(Qt::NoFocus);
    m_closeButton->setFlat(true);       //只有这个才能设置半透明效果, setStyleSheet("background:transparent;")不行

    QHBoxLayout* layout = new QHBoxLayout;
    layout->setContentsMargins(4, 0, 4, 0);
    layout->setSpacing(0);
    layout->addWidget(m_iconLabel);
    layout->addStretch(1);
    layout->addWidget(m_titleLabel);
    layout->addStretch(1);
    layout->addWidget(m_closeButton);
    setLayout(layout);

    parent->installEventFilter(this);
    connect(m_closeButton, SIGNAL(clicked()), parent, SLOT(close()));
}

bool TinyTitleBar::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::WindowTitleChange )
    {
        QWidget *pWidget = qobject_cast<QWidget *>(obj);
        if (pWidget)
        {
            m_titleLabel->setText(pWidget->windowTitle());
            return true;
        }
    }
    else if (event->type() == QEvent::WindowIconChange)
    {
        QWidget *pWidget = qobject_cast<QWidget *>(obj);
        if (pWidget)
        {
            QIcon icon = pWidget->windowIcon();
            if (!icon.isNull())
            {
                m_iconLabel->setPixmap(icon.pixmap(m_iconLabel->size()));
            }
            return true;
        }
    }
    else if (event->type()==QEvent::Resize || event->type()==QEvent::WindowStateChange )
    {
        QWidget *pWidget = qobject_cast<QWidget *>(parent());
        this->resize(pWidget->width(), this->height());
    }
    return QWidget::eventFilter(obj, event);
}

void TinyTitleBar::timerEvent(QTimerEvent *event)
{
    hide();
    killTimer(event->timerId());
    _timerId = -1;
}

void TinyTitleBar::setAutohide(int time_ms)
{
    if (_timerId<0)
    {
        if (isHidden())
            show();
        if (time_ms >0)
            _timerId = startTimer(time_ms);
    }
}

