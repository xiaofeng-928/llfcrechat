#pragma once
#include "Singleton.h"
#include <unordered_map>
#include <memory>
#include <mutex>
#include "RedisMgr.h"

class CSession;
class UserMgr: public Singleton<UserMgr>
{
	friend class Singleton<UserMgr>;
public:
	~UserMgr();
	std::shared_ptr<CSession> GetSession(int uid);
	void SetUserSession(int uid, std::shared_ptr<CSession> session);
	void RmvUserSession(int uid);
	// Redis缓存相关
	void SetUserCache(int uid, const std::string& name);
	bool GetUserCache(int uid, std::string& name);
	void RmvUserCache(int uid);
	// 定时清理不在线用户缓存
	void CheckAndCleanOfflineUsers();
private:
	UserMgr();
	std::mutex _session_mtx;
	std::unordered_map<int, std::shared_ptr<CSession>> _uid_to_session;
};
