#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "logindialog.h"
#include "registerdialog.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

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
    // 简化版界面的按钮槽函数
    void onBtnEchoClicked();
    void onBtnGetUserInfoClicked();
private:
    Ui::MainWindow *ui;
    LoginDialog* _login_dlg;
    RegisterDialog* _reg_dlg;
    QWidget* _simplified_dlg;
    QLineEdit* _uid_edit;
};

#endif // MAINWINDOW_H
