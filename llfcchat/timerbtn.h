#ifndef TIMERBTN_H
#define TIMERBTN_H

#include <QPushButton>
#include <QTimer>

class TimerBtn : public QPushButton
{
    Q_OBJECT

public:
    explicit TimerBtn(QWidget *parent = nullptr);
    ~TimerBtn();

    void startTimer(int timeout = 60);
    void stopTimer();

private slots:
    void onTimeout();

private:
    QTimer* _timer;
    int _timeout;
    int _remaining;
};

#endif // TIMERBTN_H