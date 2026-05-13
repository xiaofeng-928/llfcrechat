#include "tcpmgr.h"
#include <QAbstractSocket>
#include "usermgr.h"

TcpMgr::TcpMgr():_host(""),_port(0),_b_recv_pending(false),_message_id(0),_message_len(0)
{
    QObject::connect(&_socket, &QTcpSocket::connected, [&]() {
           qDebug() << "Connected to server!";
            emit sig_con_success(true);
       });

       QObject::connect(&_socket, &QTcpSocket::readyRead, [&]() {
           _buffer.append(_socket.readAll());

           QDataStream stream(&_buffer, QIODevice::ReadOnly);
           stream.setVersion(QDataStream::Qt_5_0);

           forever {
                if(!_b_recv_pending){
                   if (_buffer.size() < static_cast<int>(sizeof(quint16) * 2)) {
                       return;
                   }

                   stream >> _message_id >> _message_len;
                   _buffer = _buffer.mid(sizeof(quint16) * 2);
                   qDebug() << "Message ID:" << _message_id << ", Length:" << _message_len;
               }

               if(_buffer.size() < _message_len){
                    _b_recv_pending = true;
                    return;
               }

               _b_recv_pending = false;
               QByteArray messageBody = _buffer.mid(0, _message_len);
               qDebug() << "receive body msg is " << messageBody ;

               _buffer = _buffer.mid(_message_len);
               handleMsg(ReqId(_message_id),_message_len, messageBody);
           }

       });

        QObject::connect(&_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), [&](QAbstractSocket::SocketError socketError) {
            Q_UNUSED(socketError)
            qDebug() << "Error:" << _socket.errorString();
        });
        QObject::connect(&_socket, &QTcpSocket::disconnected, [&]() {
            qDebug() << "Disconnected from server.";
        });
        QObject::connect(this, &TcpMgr::sig_send_data, this, &TcpMgr::slot_send_data);
        initHandlers();
}

TcpMgr::~TcpMgr(){

}

void TcpMgr::initHandlers()
{
    _handlers.insert(ID_CHAT_LOGIN_RSP, [this](ReqId id, int len, QByteArray data){
        Q_UNUSED(len);
        qDebug()<< "handle id is "<< id ;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        if(jsonDoc.isNull()){
           qDebug() << "Failed to create QJsonDocument.";
           return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        qDebug()<< "data jsonobj is " << jsonObj ;

        if(!jsonObj.contains("error")){
            int err = ErrorCodes::ERR_JSON;
            qDebug() << "Login Failed, err is Json Parse Err" << err ;
            emit sig_login_failed(err);
            return;
        }

        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug() << "Login Failed, err is " << err ;
            emit sig_login_failed(err);
            return;
        }

        auto uid = jsonObj["uid"].toInt();
        auto name = jsonObj["name"].toString();
        auto user_info = std::make_shared<UserInfo>(uid, name, "");

        UserMgr::GetInstance()->SetUserInfo(user_info);

        emit sig_swich_chatdlg();
    });

    _handlers.insert(ID_SEND_TEXT_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        emit sig_send_text_rsp(jsonObj);
    });

    _handlers.insert(ID_NOTIFY_TEXT_CHAT_MSG_REQ, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        int from_uid = jsonObj["from_uid"].toInt();
        QString content = jsonObj["content"].toString();
        emit sig_recv_text_msg(from_uid, content);
    });

    _handlers.insert(ID_HISTORY_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("messages")) {
            qDebug() << "History Rsp missing messages field";
            return;
        }

        emit sig_history_rsp(jsonObj["messages"].toArray());
    });

    _handlers.insert(ID_AI_CHAT_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        emit sig_ai_chat_rsp(jsonObj);
    });

    _handlers.insert(ID_AI_SAVE_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        emit sig_ai_save_rsp(jsonObj);
    });

    _handlers.insert(ID_AI_HISTORY_RSP, [this](ReqId id, int len, QByteArray data) {
        Q_UNUSED(len);
        qDebug() << "handle id is " << id << " data is " << data;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if (jsonDoc.isNull()) {
            qDebug() << "Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();
        if (!jsonObj.contains("messages")) {
            qDebug() << "AI History Rsp missing messages field";
            return;
        }

        emit sig_ai_history_rsp(jsonObj["messages"].toArray());
    });
}

void TcpMgr::handleMsg(ReqId id, int len, QByteArray data)
{
   auto find_iter =  _handlers.find(id);
   if(find_iter == _handlers.end()){
        qDebug()<< "not found id ["<< id << "] to handle";
        return ;
   }

   find_iter.value()(id,len,data);
}

void TcpMgr::slot_tcp_connect(ServerInfo si)
{
    qDebug()<< "receive tcp connect signal";
    qDebug() << "Connecting to server...";
    _host = si.Host;
    _port = static_cast<uint16_t>(si.Port.toUInt());
    _socket.connectToHost(si.Host, _port);
}

void TcpMgr::slot_send_data(ReqId reqId, QByteArray dataBytes)
{
    uint16_t id = reqId;
    quint16 len = static_cast<quint16>(dataBytes.length());
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::BigEndian);
    out << id << len;
    block.append(dataBytes);
    _socket.write(block);
    qDebug() << "tcp mgr send byte data is " << block ;
}
