#ifndef HTTPMGR_H
#define HTTPMGR_H

#include "singleton.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <QJsonObject>
#include "global.h"

class HttpMgr : public QObject, public Singleton<HttpMgr>, public std::enable_shared_from_this<HttpMgr>
{
    friend class Singleton<HttpMgr>;
    Q_OBJECT

public:
    ~HttpMgr();

    void PostHttpReq(QUrl url, QJsonObject json, ReqId reqId, Modules mod);

private:
    HttpMgr();
    void initHandlers();

signals:
    void sig_reg_mod_finish(ReqId id, QString res, ErrorCodes err);
    void sig_reset_mod_finish(ReqId id, QString res, ErrorCodes err);
    void sig_login_mod_finish(ReqId id, QString res, ErrorCodes err);
    void sig_get_varifycode_finish(ReqId id, QString res, ErrorCodes err);

private slots:
    void slot_ReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager _networkAccessManager;
    QMap<ReqId, std::function<void(QJsonObject)>> _handlers;
    QMap<QNetworkReply*, std::pair<ReqId, Modules>> _req_maps;
};

#endif // HTTPMGR_H