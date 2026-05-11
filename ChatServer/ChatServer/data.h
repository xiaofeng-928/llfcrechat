#pragma once
#include <string>

struct UserInfo {
	UserInfo() : name(""), pwd(""), uid(0) {}
	int uid;
	std::string name;
	std::string pwd;
};

struct ChatMessage {
	int id;
	int from_uid;
	int to_uid;
	std::string content;
	std::string create_time;
};
