#pragma once
#include <string>

// 简化版用户信息：只有uid, name, pwd
struct UserInfo {
	UserInfo() : name(""), pwd(""), uid(0) {}
	int uid;
	std::string name;
	std::string pwd;
};
