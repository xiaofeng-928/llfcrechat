#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tcpmgr.h"
#include "usermgr.h"
#include <QLayout>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _simplified_dlg(nullptr),
    _uid_edit(nullptr)
{
    ui->setupUi(this);
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);

    //连接登录界面注册信号
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    //连接创建聊天界面信号
    connect(TcpMgr::GetInstance().get(),&TcpMgr::sig_swich_chatdlg, this, &MainWindow::SlotSwitchChat);

    // ========== 测试模式：取消下面的注释可直接跳过登录进入简化版界面 ==========
    /*
    QTimer::singleShot(100, this, [this](){
        auto test_user = std::make_shared<UserInfo>(1, "test_user", "");
        UserMgr::GetInstance()->SetUserInfo(test_user);
        emit TcpMgr::GetInstance()->sig_swich_chatdlg();
    });
    */
    // ===========================================================================
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::SlotSwitchReg()
{
    _reg_dlg = new RegisterDialog(this);
    _reg_dlg->hide();

    _reg_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);

     //连接注册界面返回登录信号
    connect(_reg_dlg, &RegisterDialog::sigSwitchLogin, this, &MainWindow::SlotSwitchLogin);
    setCentralWidget(_reg_dlg);
    _login_dlg->hide();
    _reg_dlg->show();
}

//从注册界面返回登录界面
void MainWindow::SlotSwitchLogin()
{
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);

   _reg_dlg->hide();
    _login_dlg->show();
    //连接登录界面注册信号
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
}

// 登录成功后切换到简化版界面（两个按钮）
void MainWindow::SlotSwitchChat()
{
    // 创建简化版界面
    _simplified_dlg = new QWidget();
    _simplified_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);

    QVBoxLayout* main_layout = new QVBoxLayout(_simplified_dlg);
    main_layout->setSpacing(20);
    main_layout->setContentsMargins(50, 50, 50, 50);

    // 标题
    QLabel* title = new QLabel("简化版功能界面");
    title->setAlignment(Qt::AlignCenter);
    QFont title_font = title->font();
    title_font.setPointSize(20);
    title_font.setBold(true);
    title->setFont(title_font);
    main_layout->addWidget(title);

    // 用户信息
    auto user_info = UserMgr::GetInstance()->GetUserInfo();
    QString user_text = QString("当前用户: %1 (UID: %2)")
        .arg(user_info->_name).arg(user_info->_uid);
    QLabel* user_label = new QLabel(user_text);
    user_label->setAlignment(Qt::AlignCenter);
    main_layout->addWidget(user_label);

    main_layout->addSpacing(30);

    // 按钮A：发送测试数据（Echo）
    QPushButton* btn_echo = new QPushButton("发送测试数据(Echo)");
    btn_echo->setMinimumHeight(50);
    main_layout->addWidget(btn_echo);
    connect(btn_echo, &QPushButton::clicked, this, &MainWindow::onBtnEchoClicked);

    // 按钮B：获取用户信息
    QHBoxLayout* h_layout = new QHBoxLayout();
    QLabel* uid_label = new QLabel("输入UID:");
    _uid_edit = new QLineEdit();
    _uid_edit->setPlaceholderText("请输入用户ID");
    _uid_edit->setMaximumWidth(200);
    QPushButton* btn_get_user = new QPushButton("获取用户信息");
    btn_get_user->setMinimumHeight(50);

    h_layout->addWidget(uid_label);
    h_layout->addWidget(_uid_edit);
    h_layout->addWidget(btn_get_user);
    main_layout->addLayout(h_layout);
    connect(btn_get_user, &QPushButton::clicked, this, &MainWindow::onBtnGetUserInfoClicked);

    main_layout->addStretch();

    setCentralWidget(_simplified_dlg);
    _login_dlg->hide();
    this->setMinimumSize(QSize(600,400));
    this->resize(600, 400);
    _simplified_dlg->show();
}

// 按钮A：发送测试数据
void MainWindow::onBtnEchoClicked()
{
    QString data = "Hello from client!";
    QJsonObject json;
    json["data"] = data;

    QByteArray byteArray = QJsonDocument(json).toJson(QJsonDocument::Compact);
    emit TcpMgr::GetInstance()->sig_send_data(ID_ECHO_REQ, byteArray);

    QMessageBox::information(this, "提示", "已发送Echo请求: " + data);
}

// 按钮B：获取用户信息
void MainWindow::onBtnGetUserInfoClicked()
{
    QString uid_str = _uid_edit->text();
    if(uid_str.isEmpty()) {
        QMessageBox::warning(this, "警告", "请输入用户ID");
        return;
    }

    bool ok;
    int uid = uid_str.toInt(&ok);
    if(!ok) {
        QMessageBox::warning(this, "警告", "请输入有效的数字");
        return;
    }

    QJsonObject json;
    json["uid"] = uid;

    QByteArray byteArray = QJsonDocument(json).toJson(QJsonDocument::Compact);
    emit TcpMgr::GetInstance()->sig_send_data(ID_GET_USER_INFO_REQ, byteArray);

    QMessageBox::information(this, "提示", "已发送获取用户信息请求，UID: " + uid_str);
}
