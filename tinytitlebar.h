#ifndef TINYTITLEBAR_H
#define TINYTITLEBAR_H

#include <QWidget>
#include <QPushButton>
#include <QToolButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QEvent>
#include <QApplication>

class TinyTitleBar : public QWidget
{
    Q_OBJECT
public:
    explicit TinyTitleBar(QWidget *parent = nullptr);
    void setAutohide(int time_ms);

protected:
    virtual bool eventFilter(QObject *obj, QEvent *event);
    void timerEvent(QTimerEvent *);

signals:

public slots:

private:
    QLabel* m_iconLabel;
    QLabel* m_titleLabel;
    QPushButton* m_closeButton;
//    QToolButton* m_closeButton;
    int _timerId = -1;
};

#endif // TINYTITLEBAR_H
