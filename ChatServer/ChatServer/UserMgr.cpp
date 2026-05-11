#include "UserMgr.h"
#include "CSession.h"

UserMgr::~UserMgr(){
	_uid_to_session.clear();
}

std::shared_ptr<CSession> UserMgr::GetSession(int uid)
{
	std::lock_guard<std::mutex> lock(_session_mtx);
	auto iter = _uid_to_session.find(uid);
	if (iter == _uid_to_session.end()) {
		return nullptr;
	}
	return iter->second;
}

void UserMgr::SetUserSession(int uid, std::shared_ptr<CSession> session)
{
	std::lock_guard<std::mutex> lock(_session_mtx);
	_uid_to_session[uid] = session;
}

void UserMgr::RmvUserSession(int uid)
{
	std::lock_guard<std::mutex> lock(_session_mtx);
	_uid_to_session.erase(uid);
}

UserMgr::UserMgr()
{
}

void UserMgr::CheckAndCleanOfflineUsers()
{
	std::lock_guard<std::mutex> lock(_session_mtx);

	std::vector<int> toRemove;
	for (const auto& pair : _uid_to_session) {
		int uid = pair.first;
		auto session = pair.second;
		if (!session) {
			toRemove.push_back(uid);
		}
	}

	for (int uid : toRemove) {
		_uid_to_session.erase(uid);
		std::cout << "[UserMgr] Cleaned offline user, uid=" << uid << std::endl;
	}
}
