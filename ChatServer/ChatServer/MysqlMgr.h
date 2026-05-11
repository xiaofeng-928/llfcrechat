#pragma once
#include "const.h"
#include "MysqlDao.h"
#include "Singleton.h"

class MysqlMgr: public Singleton<MysqlMgr>
{
	friend class Singleton<MysqlMgr>;
public:
	~MysqlMgr();
	// 根据uid获取用户信息
	std::shared_ptr<UserInfo> GetUser(int uid);
private:
	MysqlMgr();
	MysqlDao _dao;
};
