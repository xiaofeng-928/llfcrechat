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
#define MAX_MSG_QUE 10000

enum MSG_IDS {
	MSG_CHAT_LOGIN = 1005,
	MSG_CHAT_LOGIN_RSP = 1006,
	ID_TEXT_CHAT_MSG_REQ = 1017,
	ID_TEXT_CHAT_MSG_RSP = 1018,
	ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019,
	MSG_SEND_TEXT_REQ = 1024,
	MSG_SEND_TEXT_RSP = 1025,
	MSG_HISTORY_REQ = 1026,
	MSG_HISTORY_RSP = 1027,
};
