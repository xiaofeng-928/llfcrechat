#pragma once
#include <functional>


enum ErrorCodes {
	Success = 0,
	Error_Json = 1001,
	UidInvalid = 1011,
};

class Defer {
public:
	Defer(std::function<void()> func) : func_(func) {}
	~Defer() {
		func_();
	}
private:
	std::function<void()> func_;
};

#define MAX_LENGTH  1024*2
#define HEAD_TOTAL_LEN 4
#define HEAD_ID_LEN 2
#define HEAD_DATA_LEN 2
#define MAX_RECVQUE  10000
#define MAX_SENDQUE 1000


enum MSG_IDS {
	MSG_CHAT_LOGIN = 1005,
	MSG_CHAT_LOGIN_RSP = 1006,
	ID_SEARCH_USER_REQ = 1007,
	ID_SEARCH_USER_RSP = 1008,
	ID_ADD_FRIEND_REQ = 1009,
	ID_ADD_FRIEND_RSP  = 1010,
	ID_NOTIFY_ADD_FRIEND_REQ = 1011,
	ID_AUTH_FRIEND_REQ = 1013,
	ID_AUTH_FRIEND_RSP = 1014,
	ID_NOTIFY_AUTH_FRIEND_REQ = 1015,
	ID_TEXT_CHAT_MSG_REQ = 1017,
	ID_TEXT_CHAT_MSG_RSP = 1018,
	ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019,
	ID_ECHO_REQ = 1020,
	ID_ECHO_RSP = 1021,
	ID_GET_USER_INFO_REQ = 1022,
	ID_GET_USER_INFO_RSP = 1023,
};