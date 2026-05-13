#ifndef GLOBAL_H
#define GLOBAL_H
#include <QWidget>
#include <functional>
#include "QStyle"
#include <memory>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QDir>
#include <QSettings>

extern std::function<void(QWidget*)> repolish;

extern std::function<QString(QString)> xorString;

enum ReqId{
    ID_GET_VARIFY_CODE = 1001,
    ID_REG_USER = 1002,
    ID_RESET_PWD = 1003,
    ID_LOGIN_USER = 1004,
    ID_CHAT_LOGIN = 1005,
    ID_CHAT_LOGIN_RSP= 1006,
    ID_TEXT_CHAT_MSG_REQ  = 1017,
    ID_TEXT_CHAT_MSG_RSP  = 1018,
    ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019,
    ID_SEND_TEXT_REQ = 1024,
    ID_SEND_TEXT_RSP = 1025,
    ID_HISTORY_REQ = 1026,
    ID_HISTORY_RSP = 1027,
    ID_AI_CHAT_REQ = 1028,
    ID_AI_CHAT_RSP = 1029,
    ID_AI_HISTORY_REQ = 1030,
    ID_AI_HISTORY_RSP = 1031,
    ID_AI_SAVE_REQ = 1032,
    ID_AI_SAVE_RSP = 1033,
};

enum ErrorCodes{
    SUCCESS = 0,
    ERR_JSON = 1,
    ERR_NETWORK = 2,
};

enum Modules{
    REGISTERMOD = 0,
    RESETMOD = 1,
    LOGINMOD = 2,
};

enum TipErr{
    TIP_SUCCESS = 0,
    TIP_EMAIL_ERR = 1,
    TIP_PWD_ERR = 2,
    TIP_CONFIRM_ERR = 3,
    TIP_PWD_CONFIRM = 4,
    TIP_VARIFY_ERR = 5,
    TIP_USER_ERR = 6
};

extern QString gate_url_prefix;

struct ServerInfo{
    QString Host;
    QString Port;
    QString Token;
    int Uid;
};

#endif // GLOBAL_H
