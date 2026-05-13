#pragma once
#include "const.h"
#include "MysqlDao.h"
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "Singleton.h"

class MysqlMgr: public Singleton<MysqlMgr>
{
	friend class Singleton<MysqlMgr>;
public:
	~MysqlMgr();
	std::shared_ptr<UserInfo> GetUser(int uid);
	int SaveMessage(int from_uid, int to_uid, const std::string& content);
	std::vector<ChatMessage> GetRecentMessages(int uid, int limit);
	void AppendAiMessage(int uid, const std::string& role, const std::string& content);
	Json::Value GetAiMessages(int uid);
private:
	MysqlMgr();
	MysqlDao _dao;
};
