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
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSettings>

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
    void addSummaryBubble(const QString& text);
    void addAiUserBubble(const QString& text);
    void addAiReplyBubble(const QString& text);
    void onSendClicked();
    void onClearClicked();
    void onSummaryClicked();
    void onAiChatToggle();
    void onAiSendClicked();
    void onAiClearClicked();
    void onOpenCVClicked();
    void onSendTextRsp(QJsonObject rsp);
    void onRecvTextMsg(int from_uid, QString content);
    void onHistoryRsp(QJsonArray messages);
    void onLlmReplyFinished(QNetworkReply* reply);
    void onAiChatRsp(QJsonObject rsp);
    void onAiSaveRsp(QJsonObject rsp);
    void onAiHistoryRsp(QJsonArray messages);
    QString collectChatText();
    void lruAddMessage(const QString& role, const QString& content);
    QJsonArray lruBuildMessages();

    Ui::MainWindow *ui;
    LoginDialog* _login_dlg;
    RegisterDialog* _reg_dlg;
    QWidget* _chat_dlg;
    QListWidget* _chat_list;
    QTextEdit* _input_edit;
    QPushButton* _send_btn;
    QPushButton* _clear_btn;
    QPushButton* _summary_btn;
    QPushButton* _ai_chat_btn;
    QPushButton* _opencv_btn;
    QLabel* _title_label;
    QNetworkAccessManager* _net_mgr;
    bool _llm_busy;

    // AI 面板
    QWidget* _ai_panel;
    QListWidget* _ai_list;
    QTextEdit* _ai_input;
    QPushButton* _ai_send_btn;
    QPushButton* _ai_clear_btn;
    bool _ai_panel_visible;

    // LRU 多轮对话缓存
    struct LruEntry { QString role; QString content; };
    QList<LruEntry> _lru_messages;
    int _lru_max_size;

    // 区分 LLM 请求类型
    QString _pending_llm_type;
};

#endif // MAINWINDOW_H
