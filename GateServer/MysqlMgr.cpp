#include "MysqlMgr.h"

MysqlMgr::~MysqlMgr() {
}

MysqlMgr::MysqlMgr() {
	std::cout << "[MysqlMgr] MySQL connection pool initialized successfully!" << std::endl;
}

int MysqlMgr::RegUser(const std::string& name, const std::string& pwd) {
	return _dao.RegUser(name, pwd);
}

bool MysqlMgr::CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo) {
	return _dao.CheckPwd(name, pwd, userInfo);
}
