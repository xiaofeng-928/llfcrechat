#include "clickedlabel.h"
#include <QDebug>
#include <QFile>

ClickedLabel::ClickedLabel(QWidget *parent)
    : QLabel(parent)
{
    setCursor(Qt::PointingHandCursor);
}

ClickedLabel::~ClickedLabel()
{
}

void ClickedLabel::setClickedFunc(std::function<void()> func)
{
    _clicked_func = func;
}

void ClickedLabel::SetState(QString normal, QString hover, QString press, QString select, QString select_hover, QString selected)
{
    _normal = normal;
    _hover = hover;
    _press = press;
    _select = select;
    _select_hover = select_hover;
    _selected = selected;

    // 设置默认状态
    if(!_normal.isEmpty()) {
        QString path = QString(":/res/%1.png").arg(_normal);
        QFile file(path);
        if(file.exists()) {
            setPixmap(QPixmap(path));
        }
    }
}

QString ClickedLabel::GetCurState()
{
    return _cur_state;
}

void ClickedLabel::SetCurState(ClickLbState state)
{
    switch(state) {
    case Normal: _cur_state = _normal; break;
    case Hover: _cur_state = _hover; break;
    case Press: _cur_state = _press; break;
    case Select: _cur_state = _select; break;
    case SelectHover: _cur_state = _select_hover; break;
    case Selected: _cur_state = _selected; break;
    }
}

void ClickedLabel::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton) {
        emit clicked();
        if(_clicked_func) {
            _clicked_func();
        }
    }
    QLabel::mousePressEvent(event);
}

void ClickedLabel::mouseReleaseEvent(QMouseEvent *event)
{
    QLabel::mouseReleaseEvent(event);
}