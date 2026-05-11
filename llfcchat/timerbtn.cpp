#include "timerbtn.h"
#include <QDebug>

TimerBtn::TimerBtn(QWidget *parent)
    : QPushButton(parent)
    , _timeout(60)
    , _remaining(0)
{
    _timer = new QTimer(this);
    connect(_timer, &QTimer::timeout, this, &TimerBtn::onTimeout);
}

TimerBtn::~TimerBtn()
{
    if(_timer) {
        _timer->stop();
    }
}

void TimerBtn::startTimer(int timeout)
{
    _timeout = timeout;
    _remaining = _timeout;
    setEnabled(false);
    setText(QString("%1 s").arg(_remaining));
    _timer->start(1000);
}

void TimerBtn::stopTimer()
{
    _timer->stop();
    setEnabled(true);
    setText("获取验证码");
}

void TimerBtn::onTimeout()
{
    _remaining--;
    if(_remaining <= 0) {
        stopTimer();
    } else {
        setText(QString("%1 s").arg(_remaining));
    }
}