#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "logindialog.h"
#include "registerdialog.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QTextEdit>
#include <QJsonArray>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
public slots:
    void SlotSwitchReg();
    void SlotSwitchLogin();
    void SlotSwitchChat();
private:
    void addChatBubble(const QString& text);
    void onSendClicked();
    void onClearClicked();
    void onOpenCVClicked();
    void onSendTextRsp(QJsonObject rsp);
    void onRecvTextMsg(int from_uid, QString content);
    void onHistoryRsp(QJsonArray messages);

    Ui::MainWindow *ui;
    LoginDialog* _login_dlg;
    RegisterDialog* _reg_dlg;
    QWidget* _chat_dlg;
    QListWidget* _chat_list;
    QTextEdit* _input_edit;
    QPushButton* _send_btn;
    QPushButton* _clear_btn;
    QPushButton* _opencv_btn;
    QLabel* _title_label;
};

#endif // MAINWINDOW_H
