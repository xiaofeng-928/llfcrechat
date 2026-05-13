#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "tcpmgr.h"
#include "usermgr.h"
#include <QLayout>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QScrollBar>
#include <QTimer>
#include <QNetworkRequest>
#include <QUrl>
#include <QCoreApplication>
#include <QSplitter>

// 聊天气泡：绿色靠右
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

// 总结气泡：蓝色靠左
class SummaryBubble : public QWidget {
public:
    SummaryBubble(const QString& text, QWidget* parent = nullptr) : QWidget(parent) {
        setProperty("is_summary", true);
        QLabel* bubble = new QLabel(text);
        bubble->setWordWrap(true);
        bubble->setTextInteractionFlags(Qt::TextSelectableByMouse);
        bubble->setStyleSheet(
            "QLabel {"
            "  background-color: #D6E9FF;"
            "  border-radius: 8px;"
            "  padding: 8px 12px;"
            "  color: #000000;"
            "  font-size: 14px;"
            "}"
        );
        bubble->setMaximumWidth(450);
        bubble->adjustSize();

        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->addWidget(bubble);
        layout->addStretch(1);
        layout->setContentsMargins(8, 4, 8, 4);
    }
};

// AI 用户消息气泡：绿色靠右（和 ChatBubble 一样）
class AiUserBubble : public QWidget {
public:
    AiUserBubble(const QString& text, QWidget* parent = nullptr) : QWidget(parent) {
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
        bubble->setMaximumWidth(300);
        bubble->adjustSize();

        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->addStretch(1);
        layout->addWidget(bubble);
        layout->setContentsMargins(8, 4, 8, 4);
    }
};

// AI 回复气泡：蓝色靠左
class AiReplyBubble : public QWidget {
public:
    AiReplyBubble(const QString& text, QWidget* parent = nullptr) : QWidget(parent) {
        setProperty("is_ai_reply", true);
        QLabel* bubble = new QLabel(text);
        bubble->setWordWrap(true);
        bubble->setTextInteractionFlags(Qt::TextSelectableByMouse);
        bubble->setStyleSheet(
            "QLabel {"
            "  background-color: #D6E9FF;"
            "  border-radius: 8px;"
            "  padding: 8px 12px;"
            "  color: #000000;"
            "  font-size: 14px;"
            "}"
        );
        bubble->setMaximumWidth(300);
        bubble->adjustSize();

        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->addWidget(bubble);
        layout->addStretch(1);
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
    _summary_btn(nullptr),
    _ai_chat_btn(nullptr),
    _opencv_btn(nullptr),
    _title_label(nullptr),
    _net_mgr(nullptr),
    _llm_busy(false),
    _ai_panel(nullptr),
    _ai_list(nullptr),
    _ai_input(nullptr),
    _ai_send_btn(nullptr),
    _ai_clear_btn(nullptr),
    _ai_panel_visible(false),
    _lru_max_size(20)
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
    main_layout->setSpacing(4);
    main_layout->setContentsMargins(10, 10, 10, 10);

    // 标题栏
    auto user_info = UserMgr::GetInstance()->GetUserInfo();
    _title_label = new QLabel(QString("当前用户: %1 (UID: %2)")
        .arg(user_info->_name).arg(user_info->_uid));
    _title_label->setAlignment(Qt::AlignCenter);
    _title_label->setStyleSheet("font-size: 16px; font-weight: bold; padding: 6px;");
    main_layout->addWidget(_title_label);

    // === 左右分栏 ===
    QSplitter* split_view = new QSplitter(Qt::Horizontal);
    split_view->setObjectName("chat_splitter");
    split_view->setChildrenCollapsible(false);

    // 左侧面板：聊天
    QWidget* left_panel = new QWidget();
    QVBoxLayout* left_layout = new QVBoxLayout(left_panel);
    left_layout->setSpacing(4);
    left_layout->setContentsMargins(0, 0, 0, 0);

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
    left_layout->addWidget(_chat_list, 1);

    // 工具栏
    QHBoxLayout* toolbar_layout = new QHBoxLayout();
    _clear_btn = new QPushButton("清空记录");
    _clear_btn->setFixedHeight(32);
    _ai_chat_btn = new QPushButton("AI对话");
    _ai_chat_btn->setFixedHeight(32);
    _summary_btn = new QPushButton("总结");
    _summary_btn->setFixedHeight(32);
    _opencv_btn = new QPushButton("图像处理");
    _opencv_btn->setFixedHeight(32);
    toolbar_layout->addWidget(_clear_btn);
    toolbar_layout->addWidget(_ai_chat_btn);
    toolbar_layout->addStretch();
    toolbar_layout->addWidget(_summary_btn);
    toolbar_layout->addWidget(_opencv_btn);
    left_layout->addLayout(toolbar_layout);

    // 聊天输入栏
    QHBoxLayout* input_layout = new QHBoxLayout();
    _input_edit = new QTextEdit();
    _input_edit->setFixedHeight(50);
    _input_edit->setPlaceholderText("输入消息...");
    _send_btn = new QPushButton("发送");
    _send_btn->setFixedSize(60, 50);
    _send_btn->setStyleSheet("font-size: 14px; font-weight: bold;");
    input_layout->addWidget(_input_edit);
    input_layout->addWidget(_send_btn);
    left_layout->addLayout(input_layout);

    split_view->addWidget(left_panel);

    // 右侧面板：AI 对话（初始隐藏）
    _ai_panel = new QWidget();
    _ai_panel->hide();
    _ai_panel->setMinimumWidth(320);

    QVBoxLayout* ai_layout = new QVBoxLayout(_ai_panel);
    ai_layout->setSpacing(4);
    ai_layout->setContentsMargins(0, 0, 0, 0);

    QLabel* ai_title = new QLabel("AI 对话");
    ai_title->setAlignment(Qt::AlignCenter);
    ai_title->setStyleSheet("font-size: 14px; font-weight: bold; padding: 4px;");
    ai_layout->addWidget(ai_title);

    _ai_list = new QListWidget();
    _ai_list->setStyleSheet(
        "QListWidget {"
        "  background-color: #FAFAFA;"
        "  border: 1px solid #E0E0E0;"
        "  border-radius: 4px;"
        "}"
        "QListWidget::item {"
        "  border: none;"
        "  padding: 0px;"
        "}"
    );
    _ai_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ai_layout->addWidget(_ai_list, 1);

    _ai_clear_btn = new QPushButton("清空AI对话");
    _ai_clear_btn->setFixedHeight(28);
    ai_layout->addWidget(_ai_clear_btn);

    // AI 输入栏
    QHBoxLayout* ai_input_layout = new QHBoxLayout();
    _ai_input = new QTextEdit();
    _ai_input->setFixedHeight(50);
    _ai_input->setPlaceholderText("和AI对话...");
    _ai_send_btn = new QPushButton("发送");
    _ai_send_btn->setFixedSize(60, 50);
    _ai_send_btn->setStyleSheet("font-size: 14px; font-weight: bold;");
    ai_input_layout->addWidget(_ai_input);
    ai_input_layout->addWidget(_ai_send_btn);
    ai_layout->addLayout(ai_input_layout);

    split_view->addWidget(_ai_panel);
    split_view->setStretchFactor(0, 3);
    split_view->setStretchFactor(1, 2);
    main_layout->addWidget(split_view, 1);

    // 连接信号
    connect(_clear_btn, &QPushButton::clicked, this, &MainWindow::onClearClicked);
    connect(_ai_chat_btn, &QPushButton::clicked, this, &MainWindow::onAiChatToggle);
    connect(_summary_btn, &QPushButton::clicked, this, &MainWindow::onSummaryClicked);
    connect(_opencv_btn, &QPushButton::clicked, this, &MainWindow::onOpenCVClicked);
    connect(_send_btn, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(_ai_send_btn, &QPushButton::clicked, this, &MainWindow::onAiSendClicked);
    connect(_ai_clear_btn, &QPushButton::clicked, this, &MainWindow::onAiClearClicked);

    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_send_text_rsp,
            this, &MainWindow::onSendTextRsp);
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_recv_text_msg,
            this, &MainWindow::onRecvTextMsg);
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_history_rsp,
            this, &MainWindow::onHistoryRsp);
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_ai_chat_rsp,
            this, &MainWindow::onAiChatRsp);
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_ai_save_rsp,
            this, &MainWindow::onAiSaveRsp);
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_ai_history_rsp,
            this, &MainWindow::onAiHistoryRsp);

    _net_mgr = new QNetworkAccessManager(this);
    connect(_net_mgr, &QNetworkAccessManager::finished, this, &MainWindow::onLlmReplyFinished);

    setCentralWidget(_chat_dlg);
    _login_dlg->hide();
    this->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    this->setMinimumSize(QSize(900, 650));
    this->resize(1000, 700);
    _chat_dlg->show();

    // 加载聊天历史消息
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
    _chat_list->scrollToBottom();
}

void MainWindow::addSummaryBubble(const QString& text)
{
    SummaryBubble* bubble = new SummaryBubble(text);
    QListWidgetItem* item = new QListWidgetItem(_chat_list);
    item->setSizeHint(bubble->sizeHint());
    _chat_list->setItemWidget(item, bubble);
    _chat_list->scrollToBottom();
}

void MainWindow::addAiUserBubble(const QString& text)
{
    AiUserBubble* bubble = new AiUserBubble(text);
    QListWidgetItem* item = new QListWidgetItem(_ai_list);
    item->setSizeHint(bubble->sizeHint());
    _ai_list->setItemWidget(item, bubble);
    _ai_list->scrollToBottom();
}

void MainWindow::addAiReplyBubble(const QString& text)
{
    AiReplyBubble* bubble = new AiReplyBubble(text);
    QListWidgetItem* item = new QListWidgetItem(_ai_list);
    item->setSizeHint(bubble->sizeHint());
    _ai_list->setItemWidget(item, bubble);
    _ai_list->scrollToBottom();
}

// === LRU 缓存 ===

void MainWindow::lruAddMessage(const QString& role, const QString& content)
{
    _lru_messages.append({role, content});
    while (_lru_messages.size() > _lru_max_size) {
        _lru_messages.removeFirst();
    }
}

QJsonArray MainWindow::lruBuildMessages()
{
    QJsonArray messages;
    QJsonObject sys_msg;
    sys_msg["role"] = "system";
    sys_msg["content"] = "你是一个智能AI助手，请用中文回答问题。";
    messages.append(sys_msg);
    for (auto& entry : _lru_messages) {
        QJsonObject msg;
        msg["role"] = entry.role;
        msg["content"] = entry.content;
        messages.append(msg);
    }
    return messages;
}

// === 聊天功能 ===

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

void MainWindow::onSendTextRsp(QJsonObject rsp)
{
    int err = rsp["error"].toInt();
    if (err != 0) {
        qDebug() << "Send text failed, error: " << err;
    }
}

void MainWindow::onRecvTextMsg(int from_uid, QString content)
{
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

// === 总结功能 ===

void MainWindow::onSummaryClicked()
{
    if (_llm_busy) {
        QMessageBox::information(this, "提示", "正在处理中，请稍候...");
        return;
    }

    QString chatText = collectChatText();
    if (chatText.isEmpty()) {
        QMessageBox::information(this, "提示", "暂无聊天记录");
        return;
    }

    QDir dir(QCoreApplication::applicationDirPath());
    QString config_path = dir.absoluteFilePath("config.ini");
    while (!QFile::exists(config_path) && dir.cdUp()) {
        config_path = dir.absoluteFilePath("config.ini");
    }
    QSettings settings(config_path, QSettings::IniFormat);
    QString api_key = settings.value("LLM/ApiKey").toString();
    QString api_url = settings.value("LLM/ApiUrl").toString();
    QString model = settings.value("LLM/Model").toString();

    if (api_key.isEmpty() || api_url.isEmpty()) {
        QMessageBox::warning(this, "错误", "LLM 配置缺失，请检查 config.ini");
        return;
    }

    QJsonObject sys_msg;
    sys_msg["role"] = "system";
    sys_msg["content"] = "你是一个聊天记录总结助手。请用简洁的中文总结以下聊天记录的要点。";

    QJsonObject user_msg;
    user_msg["role"] = "user";
    user_msg["content"] = chatText;

    QJsonArray messages;
    messages.append(sys_msg);
    messages.append(user_msg);

    QJsonObject req_body;
    req_body["model"] = model;
    req_body["stream"] = false;
    req_body["messages"] = messages;

    QJsonDocument doc(req_body);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    QNetworkRequest request{QUrl(api_url)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + api_key).toUtf8());

    _llm_busy = true;
    _summary_btn->setEnabled(false);
    _pending_llm_type = "summary";
    addSummaryBubble("正在总结...");

    _net_mgr->post(request, data);
}

void MainWindow::onLlmReplyFinished(QNetworkReply* reply)
{
    _llm_busy = false;
    _summary_btn->setEnabled(true);
    _ai_send_btn->setEnabled(true);

    if (_pending_llm_type == "summary") {
        if (_chat_list->count() > 0) {
            int last = _chat_list->count() - 1;
            QWidget* widget = _chat_list->itemWidget(_chat_list->item(last));
            if (widget && widget->property("is_summary").toBool()) {
                delete _chat_list->takeItem(last);
            }
        }

        if (reply->error() != QNetworkReply::NoError) {
            addSummaryBubble("总结请求失败: " + reply->errorString());
            reply->deleteLater();
            return;
        }

        QByteArray response = reply->readAll();
        reply->deleteLater();

        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isNull()) {
            addSummaryBubble("总结结果解析失败");
            return;
        }

        QJsonArray choices = doc.object()["choices"].toArray();
        if (choices.isEmpty()) {
            addSummaryBubble("总结结果为空");
            return;
        }

        QString summary = choices[0].toObject()["message"].toObject()["content"].toString();
        addSummaryBubble(summary.isEmpty() ? "总结结果为空" : summary);

    } else if (_pending_llm_type == "ai_chat") {
        // 移除"思考中..."占位气泡
        if (_ai_list->count() > 0) {
            int last = _ai_list->count() - 1;
            QWidget* widget = _ai_list->itemWidget(_ai_list->item(last));
            if (widget && widget->property("is_ai_reply").toBool()) {
                delete _ai_list->takeItem(last);
            }
        }

        if (reply->error() != QNetworkReply::NoError) {
            addAiReplyBubble("AI请求失败: " + reply->errorString());
            reply->deleteLater();
            return;
        }

        QByteArray response = reply->readAll();
        reply->deleteLater();

        QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isNull()) {
            addAiReplyBubble("AI回复解析失败");
            return;
        }

        QJsonArray choices = doc.object()["choices"].toArray();
        if (choices.isEmpty()) {
            addAiReplyBubble("AI回复为空");
            return;
        }

        QString ai_content = choices[0].toObject()["message"].toObject()["content"].toString();
        if (ai_content.isEmpty()) {
            addAiReplyBubble("AI回复为空");
        } else {
            addAiReplyBubble(ai_content);
            lruAddMessage("assistant", ai_content);

            // 保存 AI 回复到服务器数据库
            QJsonObject save_json;
            save_json["content"] = ai_content;
            QByteArray saveData = QJsonDocument(save_json).toJson(QJsonDocument::Compact);
            emit TcpMgr::GetInstance()->sig_send_data(ID_AI_SAVE_REQ, saveData);
        }
    }
}

QString MainWindow::collectChatText()
{
    QString result;
    for (int i = 0; i < _chat_list->count(); ++i) {
        QListWidgetItem* item = _chat_list->item(i);
        QWidget* widget = _chat_list->itemWidget(item);
        if (!widget) continue;

        QHBoxLayout* layout = qobject_cast<QHBoxLayout*>(widget->layout());
        if (!layout) continue;

        for (int j = 0; j < layout->count(); ++j) {
            QLabel* label = qobject_cast<QLabel*>(layout->itemAt(j)->widget());
            if (label && !label->text().isEmpty()) {
                result += label->text() + "\n";
                break;
            }
        }
    }
    return result.trimmed();
}

// === AI 对话功能 ===

void MainWindow::onAiChatToggle()
{
    auto split_view = _chat_dlg ? _chat_dlg->findChild<QSplitter*>("chat_splitter") : nullptr;
    _ai_panel_visible = !_ai_panel_visible;
    if (_ai_panel_visible) {
        const int min_chat_width_with_ai = 1280;
        this->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        this->setMinimumSize(QSize(min_chat_width_with_ai, 650));
        if (this->width() < min_chat_width_with_ai) {
            this->resize(min_chat_width_with_ai, this->height());
        }

        _ai_panel->show();
        if (split_view) {
            // 等界面完成一次布局后再设置比例，避免 show() 当帧尺寸未更新导致分配失败
            QTimer::singleShot(0, this, [this, split_view]() {
                if (!_ai_panel_visible || !_ai_panel || !split_view) return;
                const int total = qMax(split_view->width(), 1);
                QList<int> sizes;
                sizes << (total * 52 / 100) << (total * 48 / 100);
                split_view->setSizes(sizes);
            });
        }
        _ai_chat_btn->setText("关闭AI");

        // 加载 AI 对话历史
        QJsonObject json;
        json["limit"] = 50;
        QByteArray byteArray = QJsonDocument(json).toJson(QJsonDocument::Compact);
        emit TcpMgr::GetInstance()->sig_send_data(ID_AI_HISTORY_REQ, byteArray);
    } else {
        if (split_view) {
            QList<int> sizes;
            sizes << qMax(split_view->width(), 1) << 0;
            split_view->setSizes(sizes);
        }
        this->setMinimumSize(QSize(900, 650));
        _ai_panel->hide();
        _ai_chat_btn->setText("AI对话");
    }
}

void MainWindow::onAiSendClicked()
{
    QString text = _ai_input->toPlainText().trimmed();
    if (text.isEmpty()) {
        return;
    }

    if (_llm_busy) {
        QMessageBox::information(this, "提示", "正在处理中，请稍候...");
        return;
    }

    addAiUserBubble(text);
    lruAddMessage("user", text);
    _ai_input->clear();

    // 1. 保存用户消息到服务器数据库
    QJsonObject save_json;
    save_json["content"] = text;
    QByteArray saveData = QJsonDocument(save_json).toJson(QJsonDocument::Compact);
    emit TcpMgr::GetInstance()->sig_send_data(ID_AI_CHAT_REQ, saveData);

    // 2. 客户端直接调用 LLM
    QDir dir(QCoreApplication::applicationDirPath());
    QString config_path = dir.absoluteFilePath("config.ini");
    while (!QFile::exists(config_path) && dir.cdUp()) {
        config_path = dir.absoluteFilePath("config.ini");
    }
    QSettings settings(config_path, QSettings::IniFormat);
    QString api_key = settings.value("LLM/ApiKey").toString();
    QString api_url = settings.value("LLM/ApiUrl").toString();
    QString model = settings.value("LLM/Model").toString();

    if (api_key.isEmpty() || api_url.isEmpty()) {
        addAiReplyBubble("LLM 配置缺失，请检查 config.ini");
        return;
    }

    QJsonArray messages = lruBuildMessages();
    QJsonObject req_body;
    req_body["model"] = model;
    req_body["stream"] = false;
    req_body["messages"] = messages;

    QJsonDocument doc(req_body);
    QByteArray data = doc.toJson(QJsonDocument::Compact);

    QNetworkRequest request{QUrl(api_url)};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + api_key).toUtf8());

    _llm_busy = true;
    _ai_send_btn->setEnabled(false);
    _summary_btn->setEnabled(false);
    _pending_llm_type = "ai_chat";
    addAiReplyBubble("思考中...");

    _net_mgr->post(request, data);
}

void MainWindow::onAiChatRsp(QJsonObject rsp)
{
    // 服务器确认用户消息已保存，无需特殊处理
    Q_UNUSED(rsp);
}

void MainWindow::onAiSaveRsp(QJsonObject rsp)
{
    // 服务器确认AI回复已保存，无需特殊处理
    Q_UNUSED(rsp);
}

void MainWindow::onAiHistoryRsp(QJsonArray messages)
{
    _ai_list->clear();
    _lru_messages.clear();

    for (const QJsonValue& val : messages) {
        QJsonObject obj = val.toObject();
        QString role = obj["role"].toString();
        QString content = obj["content"].toString();

        if (role == "user") {
            addAiUserBubble(content);
        } else if (role == "assistant") {
            addAiReplyBubble(content);
        }

        lruAddMessage(role, content);
    }
}

void MainWindow::onAiClearClicked()
{
    _ai_list->clear();
    _lru_messages.clear();
}

void MainWindow::onOpenCVClicked()
{
    QMessageBox::information(this, "提示", "图像处理功能开发中...");
}
