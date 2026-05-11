#include "MysqlMgr.h"

MysqlMgr::~MysqlMgr() {
}

MysqlMgr::MysqlMgr() {
	std::cout << "[MysqlMgr] MySQL connection pool initialized successfully!" << std::endl;
}

std::shared_ptr<UserInfo> MysqlMgr::GetUser(int uid) {
	return _dao.GetUser(uid);
}

int MysqlMgr::SaveMessage(int from_uid, int to_uid, const std::string& content) {
	return _dao.SaveMessage(from_uid, to_uid, content);
}

std::vector<ChatMessage> MysqlMgr::GetRecentMessages(int uid, int limit) {
	return _dao.GetRecentMessages(uid, limit);
}
