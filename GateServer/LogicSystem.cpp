#include "LogicSystem.h"
#include "HttpConnection.h"
#include "MysqlMgr.h"
#include "ConfigMgr.h"

LogicSystem::LogicSystem() {
	// 用户注册：用户名 + 密码，直接写入MySQL
	RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto name = src_root["user"].asString();
		auto pwd = src_root["passwd"].asString();
		auto confirm = src_root["confirm"].asString();

		if (pwd != confirm) {
			std::cout << "password err " << std::endl;
			root["error"] = ErrorCodes::PasswdErr;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		// 直接写入数据库，不验证验证码
		int uid = MysqlMgr::GetInstance()->RegUser(name, pwd);
		if (uid == 0 || uid == -1) {
			std::cout << " user or email exist" << std::endl;
			root["error"] = ErrorCodes::UserExist;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		root["error"] = 0;
		root["uid"] = uid;
		root["user"] = name;
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
	});

	// 用户登录：用户名 + 密码
	RegPost("/user_login", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto name = src_root["user"].asString();
		auto pwd = src_root["passwd"].asString();
		UserInfo userInfo;
		// 查询数据库判断用户和密码是否匹配
		bool pwd_valid = MysqlMgr::GetInstance()->CheckPwd(name, pwd, userInfo);
		if (!pwd_valid) {
			std::cout << " user pwd not match" << std::endl;
			root["error"] = ErrorCodes::PasswdInvalid;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		// 从配置文件获取ChatServer信息
		auto& cfg = ConfigMgr::Inst();
		std::string chat_host = cfg["ChatServer"]["Host"];
		std::string chat_port = cfg["ChatServer"]["Port"];

		std::cout << "succeed to load userinfo uid is " << userInfo.uid << std::endl;
		root["error"] = 0;
		root["uid"] = userInfo.uid;
		root["user"] = userInfo.name;
		root["host"] = chat_host;
		root["port"] = chat_port;
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
	});
}

void LogicSystem::RegGet(std::string url, HttpHandler handler) {
	_get_handlers.insert(make_pair(url, handler));
}

void LogicSystem::RegPost(std::string url, HttpHandler handler) {
	_post_handlers.insert(make_pair(url, handler));
}

LogicSystem::~LogicSystem() {

}

bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> con) {
	if (_get_handlers.find(path) == _get_handlers.end()) {
		return false;
	}

	_get_handlers[path](con);
	return true;
}

bool LogicSystem::HandlePost(std::string path, std::shared_ptr<HttpConnection> con) {
	if (_post_handlers.find(path) == _post_handlers.end()) {
		return false;
	}

	_post_handlers[path](con);
	return true;
}