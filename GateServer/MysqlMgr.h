#pragma once
#include "const.h"
#include "MysqlDao.h"

class MysqlMgr: public Singleton<MysqlMgr>
{
	friend class Singleton<MysqlMgr>;
public:
	~MysqlMgr();
	// 注册用户：用户名 + 密码
	int RegUser(const std::string& name, const std::string& pwd);
	// 验证密码
	bool CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo);
private:
	MysqlMgr();
	MysqlDao _dao;
};
