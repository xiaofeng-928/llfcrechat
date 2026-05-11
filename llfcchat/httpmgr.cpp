#include "httpmgr.h"
#include <QJsonDocument>
#include <QDebug>

HttpMgr::HttpMgr() {
    initHandlers();
    connect(&_networkAccessManager, &QNetworkAccessManager::finished,
            this, &HttpMgr::slot_ReplyFinished);
}

HttpMgr::~HttpMgr() {
}

void HttpMgr::initHandlers() {
    _handlers[ReqId::ID_REG_USER] = [this](QJsonObject jsonObj) {
        emit sig_reg_mod_finish(ReqId::ID_REG_USER,
            QString::fromUtf8(QJsonDocument(jsonObj).toJson(QJsonDocument::Compact)),
            ErrorCodes::SUCCESS);
    };

    _handlers[ReqId::ID_RESET_PWD] = [this](QJsonObject jsonObj) {
        emit sig_reset_mod_finish(ReqId::ID_RESET_PWD,
            QString::fromUtf8(QJsonDocument(jsonObj).toJson(QJsonDocument::Compact)),
            ErrorCodes::SUCCESS);
    };

    _handlers[ReqId::ID_LOGIN_USER] = [this](QJsonObject jsonObj) {
        emit sig_login_mod_finish(ReqId::ID_LOGIN_USER,
            QString::fromUtf8(QJsonDocument(jsonObj).toJson(QJsonDocument::Compact)),
            ErrorCodes::SUCCESS);
    };

    _handlers[ReqId::ID_GET_VARIFY_CODE] = [this](QJsonObject jsonObj) {
        emit sig_get_varifycode_finish(ReqId::ID_GET_VARIFY_CODE,
            QString::fromUtf8(QJsonDocument(jsonObj).toJson(QJsonDocument::Compact)),
            ErrorCodes::SUCCESS);
    };
}

void HttpMgr::PostHttpReq(QUrl url, QJsonObject json, ReqId reqId, Modules mod) {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);
    QNetworkReply* reply = _networkAccessManager.post(request, data);

    _req_maps[reply] = std::make_pair(reqId, mod);
}

void HttpMgr::slot_ReplyFinished(QNetworkReply *reply) {
    if(reply->error() != QNetworkReply::NoError) {
        qDebug() << "http reply error: " << reply->errorString();
        reply->deleteLater();
        return;
    }

    auto it = _req_maps.find(reply);
    if(it == _req_maps.end()) {
        reply->deleteLater();
        return;
    }

    auto reqId = it->first;
    auto mod = it->second;
    _req_maps.erase(it);

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject jsonObj = doc.object();

    int error = jsonObj["error"].toInt();
    if(error != ErrorCodes::SUCCESS) {
        emit sig_reg_mod_finish(reqId, QString::fromUtf8(data), (ErrorCodes)error);
        emit sig_reset_mod_finish(reqId, QString::fromUtf8(data), (ErrorCodes)error);
        emit sig_login_mod_finish(reqId, QString::fromUtf8(data), (ErrorCodes)error);
        emit sig_get_varifycode_finish(reqId, QString::fromUtf8(data), (ErrorCodes)error);
    } else {
        auto find_iter = _handlers.find(reqId);
        if(find_iter != _handlers.end()) {
            find_iter.value()(jsonObj);
        }
    }

    reply->deleteLater();
}