#include "LogicSystem.h"
#include "MysqlMgr.h"
#include "const.h"
#include "UserMgr.h"
#include <chrono>

using namespace std;

LogicSystem::LogicSystem():_b_stop(false){
	RegisterCallBacks();
	_worker_thread = std::thread (&LogicSystem::DealMsg, this);

	std::thread clean_thread([this]() {
		while (!_b_stop) {
			std::this_thread::sleep_for(std::chrono::minutes(5));
			if (!_b_stop) {
				UserMgr::GetInstance()->CheckAndCleanOfflineUsers();
			}
		}
	});
	clean_thread.detach();
}

LogicSystem::~LogicSystem(){
	_b_stop = true;
	_consume.notify_one();
	_worker_thread.join();
}

void LogicSystem::PostMsgToQue(shared_ptr < LogicNode> msg) {
	std::unique_lock<std::mutex> unique_lk(_mutex);
	if (_msg_que.size() >= MAX_MSG_QUE) {
		std::cerr << "msg queue is full, dropping message" << std::endl;
		return;
	}
	_msg_que.push(msg);
	if (_msg_que.size() == 1) {
		unique_lk.unlock();
		_consume.notify_one();
	}
}

void LogicSystem::DealMsg() {
	for (;;) {
		std::unique_lock<std::mutex> unique_lk(_mutex);
		while (_msg_que.empty() && !_b_stop) {
			_consume.wait(unique_lk);
		}

		if (_b_stop ) {
			while (!_msg_que.empty()) {
				auto msg_node = _msg_que.front();
				std::cout << "recv_msg id  is " << msg_node->_recvnode->_msg_id << std::endl;
				auto call_back_iter = _fun_callbacks.find(msg_node->_recvnode->_msg_id);
				if (call_back_iter == _fun_callbacks.end()) {
					_msg_que.pop();
					continue;
				}
				call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id,
					std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
				_msg_que.pop();
			}
			break;
		}

		auto msg_node = _msg_que.front();
		std::cout << "recv_msg id  is " << msg_node->_recvnode->_msg_id << std::endl;
		auto call_back_iter = _fun_callbacks.find(msg_node->_recvnode->_msg_id);
		if (call_back_iter == _fun_callbacks.end()) {
			_msg_que.pop();
			std::cout << "msg id [" << msg_node->_recvnode->_msg_id << "] handler not found" << std::endl;
			continue;
		}
		call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id,
			std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len));
		_msg_que.pop();
	}
}

void LogicSystem::RegisterCallBacks() {
	_fun_callbacks[MSG_CHAT_LOGIN] = std::bind(&LogicSystem::LoginHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[MSG_SEND_TEXT_REQ] = std::bind(&LogicSystem::SendTextHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);

	_fun_callbacks[MSG_HISTORY_REQ] = std::bind(&LogicSystem::HistoryHandler, this,
		placeholders::_1, placeholders::_2, placeholders::_3);
}

void LogicSystem::LoginHandler(shared_ptr<CSession> session, const short &msg_id, const string &msg_data) {
	Json::Reader reader;
	Json::Value root;
	if (!reader.parse(msg_data, root)) {
		Json::Value rtvalue;
		rtvalue["error"] = ErrorCodes::Error_Json;
		session->Send(rtvalue.toStyledString(), MSG_CHAT_LOGIN_RSP);
		return;
	}

	auto uid = root["uid"].asInt();
	std::cout << "user login uid is  " << uid << std::endl;

	Json::Value rtvalue;
	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, MSG_CHAT_LOGIN_RSP);
	});

	auto user_info = MysqlMgr::GetInstance()->GetUser(uid);
	if (user_info == nullptr) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}

	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["uid"] = uid;
	rtvalue["name"] = user_info->name;

	session->SetUserId(uid);
	UserMgr::GetInstance()->SetUserSession(uid, session);
}

void LogicSystem::SendTextHandler(shared_ptr<CSession> session, const short &msg_id, const string &msg_data) {
	Json::Reader reader;
	Json::Value root;
	Json::Value rtvalue;

	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, MSG_SEND_TEXT_RSP);
	});

	if (!reader.parse(msg_data, root)) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}

	int from_uid = session->GetUserId();
	int to_uid = root["to_uid"].asInt();
	std::string content = root["content"].asString();

	int msg_id_db = MysqlMgr::GetInstance()->SaveMessage(from_uid, to_uid, content);
	if (msg_id_db < 0) {
		rtvalue["error"] = ErrorCodes::UidInvalid;
		return;
	}

	rtvalue["error"] = ErrorCodes::Success;
	rtvalue["msg_id"] = msg_id_db;

	// 尝试转发给对方
	auto to_session = UserMgr::GetInstance()->GetSession(to_uid);
	if (to_session != nullptr) {
		Json::Value notify;
		notify["from_uid"] = from_uid;
		notify["content"] = content;
		to_session->Send(notify.toStyledString(), ID_NOTIFY_TEXT_CHAT_MSG_REQ);
	}
}

void LogicSystem::HistoryHandler(shared_ptr<CSession> session, const short &msg_id, const string &msg_data) {
	Json::Reader reader;
	Json::Value root;
	Json::Value rtvalue;

	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, MSG_HISTORY_RSP);
	});

	if (!reader.parse(msg_data, root)) {
		rtvalue["error"] = ErrorCodes::Error_Json;
		return;
	}

	int uid = session->GetUserId();
	int limit = root.get("limit", 50).asInt();

	auto messages = MysqlMgr::GetInstance()->GetRecentMessages(uid, limit);

	rtvalue["error"] = ErrorCodes::Success;
	Json::Value msg_array(Json::arrayValue);
	for (const auto& msg : messages) {
		Json::Value item;
		item["id"] = msg.id;
		item["from_uid"] = msg.from_uid;
		item["to_uid"] = msg.to_uid;
		item["content"] = msg.content;
		item["create_time"] = msg.create_time;
		msg_array.append(item);
	}
	rtvalue["messages"] = msg_array;
}
