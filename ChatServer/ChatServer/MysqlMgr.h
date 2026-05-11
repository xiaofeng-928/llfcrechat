#pragma once
#include "const.h"
#include "MysqlDao.h"
#include "Singleton.h"

class MysqlMgr: public Singleton<MysqlMgr>
{
	friend class Singleton<MysqlMgr>;
public:
	~MysqlMgr();
	std::shared_ptr<UserInfo> GetUser(int uid);
	int SaveMessage(int from_uid, int to_uid, const std::string& content);
	std::vector<ChatMessage> GetRecentMessages(int uid, int limit);
private:
	MysqlMgr();
	MysqlDao _dao;
};
