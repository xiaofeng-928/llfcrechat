#ifndef USERDATA_H
#define USERDATA_H
#include <QString>
#include <memory>

struct UserInfo {
    UserInfo(int uid, QString name, QString icon):
    _uid(uid), _name(name), _icon(icon),_nick(_name),
    _sex(0),_last_msg(""){

    }

    int _uid;
    QString _name;
    QString _nick;
    QString _icon;
    int _sex;
    QString _last_msg;
};

#endif
