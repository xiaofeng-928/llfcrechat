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
	{
		std::lock_guard<std::mutex> lock(_session_mtx);
		_uid_to_session.erase(uid);
	}
	// 删除Redis缓存
	RmvUserCache(uid);
}

UserMgr::UserMgr()
{
}

// 保存用户到Redis缓存
void UserMgr::SetUserCache(int uid, const std::string& name)
{
	std::string key = "user_session_" + std::to_string(uid);
	RedisMgr::GetInstance()->Set(key, name);
}

// 从Redis缓存获取用户
bool UserMgr::GetUserCache(int uid, std::string& name)
{
	std::string key = "user_session_" + std::to_string(uid);
	return RedisMgr::GetInstance()->Get(key, name);
}

// 删除Redis缓存
void UserMgr::RmvUserCache(int uid)
{
	std::string key = "user_session_" + std::to_string(uid);
	RedisMgr::GetInstance()->Del(key);
}

// 检查并清理不在线的用户缓存（每5分钟调用一次）
void UserMgr::CheckAndCleanOfflineUsers()
{
	std::lock_guard<std::mutex> lock(_session_mtx);

	// 遍历当前在线用户，如果session已被移除则清理缓存
	std::vector<int> toRemove;
	for (const auto& pair : _uid_to_session) {
		int uid = pair.first;
		auto session = pair.second;
		// 如果session为空或已断开连接
		if (!session) {
			toRemove.push_back(uid);
		}
	}

	// 清理离线用户的Redis缓存
	for (int uid : toRemove) {
		_uid_to_session.erase(uid);
		RmvUserCache(uid);
		std::cout << "[UserMgr] Cleaned offline user cache, uid=" << uid << std::endl;
	}
}
