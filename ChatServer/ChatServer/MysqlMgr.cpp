#include "MysqlMgr.h"

MysqlMgr::~MysqlMgr() {
}

MysqlMgr::MysqlMgr() {
	std::cout << "[MysqlMgr] MySQL connection pool initialized successfully!" << std::endl;
}

std::shared_ptr<UserInfo> MysqlMgr::GetUser(int uid) {
	return _dao.GetUser(uid);
}
