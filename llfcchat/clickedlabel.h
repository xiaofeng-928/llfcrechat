#ifndef CLICKEDLABEL_H
#define CLICKEDLABEL_H

#include <QLabel>
#include <QMouseEvent>
#include <QString>
#include <functional>

// 删掉多余的前置声明 + 重复typedef
class ClickedLabel : public QLabel
{
    Q_OBJECT

public:
    enum ClickLbState {
        Normal = 0,
        Hover = 1,
        Press = 2,
        Select = 3,
        SelectHover = 4,
        Selected = 5
    };

    explicit ClickedLabel(QWidget *parent = nullptr);
    ~ClickedLabel();

    void setClickedFunc(std::function<void()> func);
    void SetState(QString normal, QString hover, QString press, QString select, QString select_hover, QString selected);
    QString GetCurState();
    void SetCurState(ClickLbState state);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    std::function<void()> _clicked_func;
    QString _cur_state;
    QString _normal;
    QString _hover;
    QString _press;
    QString _select;
    QString _select_hover;
    QString _selected;
};

// 只保留这一个typedef，且放在类定义之后
//typedef ClickedLabel::ClickLbState ClickLbState;

#endif // CLICKEDLABEL_H
