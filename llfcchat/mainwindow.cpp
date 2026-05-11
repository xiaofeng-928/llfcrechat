#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tcpmgr.h"
#include "usermgr.h"
#include <QLayout>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScrollBar>
#include <QTimer>

// 聊天气泡 Widget：消息靠右显示，浅绿色背景
class ChatBubble : public QWidget {
public:
    ChatBubble(const QString& text, QWidget* parent = nullptr) : QWidget(parent) {
        QLabel* bubble = new QLabel(text);
        bubble->setWordWrap(true);
        bubble->setTextInteractionFlags(Qt::TextSelectableByMouse);
        bubble->setStyleSheet(
            "QLabel {"
            "  background-color: #95EC69;"
            "  border-radius: 8px;"
            "  padding: 8px 12px;"
            "  color: #000000;"
            "  font-size: 14px;"
            "}"
        );
        bubble->setMaximumWidth(400);
        bubble->adjustSize();

        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->addStretch(1);
        layout->addWidget(bubble);
        layout->setContentsMargins(8, 4, 8, 4);
    }
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _chat_dlg(nullptr),
    _chat_list(nullptr),
    _input_edit(nullptr),
    _send_btn(nullptr),
    _clear_btn(nullptr),
    _opencv_btn(nullptr),
    _title_label(nullptr)
{
    ui->setupUi(this);
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);

    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_swich_chatdlg, this, &MainWindow::SlotSwitchChat);
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
    connect(_reg_dlg, &RegisterDialog::sigSwitchLogin, this, &MainWindow::SlotSwitchLogin);
    setCentralWidget(_reg_dlg);
    _login_dlg->hide();
    _reg_dlg->show();
}

void MainWindow::SlotSwitchLogin()
{
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);
    _reg_dlg->hide();
    _login_dlg->show();
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
}

void MainWindow::SlotSwitchChat()
{
    _chat_dlg = new QWidget();
    _chat_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);

    QVBoxLayout* main_layout = new QVBoxLayout(_chat_dlg);
    main_layout->setSpacing(8);
    main_layout->setContentsMargins(10, 10, 10, 10);

    // 标题栏
    auto user_info = UserMgr::GetInstance()->GetUserInfo();
    _title_label = new QLabel(QString("当前用户: %1 (UID: %2)")
        .arg(user_info->_name).arg(user_info->_uid));
    _title_label->setAlignment(Qt::AlignCenter);
    _title_label->setStyleSheet("font-size: 16px; font-weight: bold; padding: 6px;");
    main_layout->addWidget(_title_label);

    // 消息显示区
    _chat_list = new QListWidget();
    _chat_list->setStyleSheet(
        "QListWidget {"
        "  background-color: #F5F5F5;"
        "  border: 1px solid #E0E0E0;"
        "  border-radius: 4px;"
        "}"
        "QListWidget::item {"
        "  border: none;"
        "  padding: 0px;"
        "}"
    );
    _chat_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    main_layout->addWidget(_chat_list, 1);

    // 工具栏
    QHBoxLayout* toolbar_layout = new QHBoxLayout();
    _clear_btn = new QPushButton("清空记录");
    _clear_btn->setFixedHeight(32);
    _opencv_btn = new QPushButton("图像处理");
    _opencv_btn->setFixedHeight(32);
    toolbar_layout->addWidget(_clear_btn);
    toolbar_layout->addStretch();
    toolbar_layout->addWidget(_opencv_btn);
    main_layout->addLayout(toolbar_layout);

    connect(_clear_btn, &QPushButton::clicked, this, &MainWindow::onClearClicked);
    connect(_opencv_btn, &QPushButton::clicked, this, &MainWindow::onOpenCVClicked);

    // 输入栏
    QHBoxLayout* input_layout = new QHBoxLayout();
    _input_edit = new QTextEdit();
    _input_edit->setFixedHeight(60);
    _input_edit->setPlaceholderText("输入消息...");
    _send_btn = new QPushButton("发送");
    _send_btn->setFixedSize(80, 60);
    _send_btn->setStyleSheet("font-size: 16px; font-weight: bold;");
    input_layout->addWidget(_input_edit);
    input_layout->addWidget(_send_btn);
    main_layout->addLayout(input_layout);

    connect(_send_btn, &QPushButton::clicked, this, &MainWindow::onSendClicked);

    // 连接信号
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_send_text_rsp,
            this, &MainWindow::onSendTextRsp);
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_recv_text_msg,
            this, &MainWindow::onRecvTextMsg);
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_history_rsp,
            this, &MainWindow::onHistoryRsp);

    setCentralWidget(_chat_dlg);
    _login_dlg->hide();
    this->setMinimumSize(QSize(500, 600));
    this->resize(600, 700);
    _chat_dlg->show();

    // 加载历史消息
    QJsonObject json;
    json["limit"] = 50;
    QByteArray byteArray = QJsonDocument(json).toJson(QJsonDocument::Compact);
    emit TcpMgr::GetInstance()->sig_send_data(ID_HISTORY_REQ, byteArray);
}

void MainWindow::addChatBubble(const QString& text)
{
    ChatBubble* bubble = new ChatBubble(text);
    QListWidgetItem* item = new QListWidgetItem(_chat_list);
    item->setSizeHint(bubble->sizeHint());
    _chat_list->setItemWidget(item, bubble);

    // 自动滚动到底部
    _chat_list->scrollToBottom();
}

void MainWindow::onSendClicked()
{
    QString text = _input_edit->toPlainText().trimmed();
    if (text.isEmpty()) {
        return;
    }

    int to_uid = UserMgr::GetInstance()->GetUid();

    QJsonObject json;
    json["to_uid"] = to_uid;
    json["content"] = text;

    QByteArray byteArray = QJsonDocument(json).toJson(QJsonDocument::Compact);
    emit TcpMgr::GetInstance()->sig_send_data(ID_SEND_TEXT_REQ, byteArray);

    addChatBubble(text);
    _input_edit->clear();
}

void MainWindow::onClearClicked()
{
    _chat_list->clear();
}

void MainWindow::onOpenCVClicked()
{
    QMessageBox::information(this, "提示", "图像处理功能开发中...");
}

void MainWindow::onSendTextRsp(QJsonObject rsp)
{
    int err = rsp["error"].toInt();
    if (err != 0) {
        qDebug() << "Send text failed, error: " << err;
    }
}

void MainWindow::onRecvTextMsg(int from_uid, QString content)
{
    // 跳过自己发的消息通知（onSendClicked已经显示了）
    if (from_uid == UserMgr::GetInstance()->GetUid()) {
        return;
    }
    addChatBubble(content);
}

void MainWindow::onHistoryRsp(QJsonArray messages)
{
    for (const QJsonValue& val : messages) {
        QJsonObject obj = val.toObject();
        addChatBubble(obj["content"].toString());
    }
}
