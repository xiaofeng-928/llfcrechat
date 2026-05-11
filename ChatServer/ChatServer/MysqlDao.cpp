#include "MysqlDao.h"
#include "ConfigMgr.h"

MysqlDao::MysqlDao()
{
	auto & cfg = ConfigMgr::Inst();
	const auto& host = cfg["Mysql"]["Host"];
	const auto& port = cfg["Mysql"]["Port"];
	const auto& pwd = cfg["Mysql"]["Passwd"];
	const auto& schema = cfg["Mysql"]["Schema"];
	const auto& user = cfg["Mysql"]["User"];
	pool_.reset(new MySqlPool(host+":"+port, user, pwd, schema, 5));
	std::cout << "[MysqlDao] Connection pool created" << std::endl;
}

MysqlDao::~MysqlDao(){
	pool_->Close();
}

std::shared_ptr<UserInfo> MysqlDao::GetUser(int uid)
{
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return nullptr;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
	});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt_user(con->_con->prepareStatement("SELECT uid, name, pwd FROM user WHERE uid = ?"));
		pstmt_user->setInt(1, uid);

		std::unique_ptr<sql::ResultSet> res(pstmt_user->executeQuery());
		std::shared_ptr<UserInfo> user_ptr = nullptr;

		if (res->next()) {
			user_ptr = std::make_shared<UserInfo>();
			user_ptr->uid = res->getInt("uid");
			user_ptr->name = res->getString("name");
			user_ptr->pwd = res->getString("pwd");
		}
		return user_ptr;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return nullptr;
	}
}

int MysqlDao::SaveMessage(int from_uid, int to_uid, const std::string& content)
{
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return -1;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
	});

	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"INSERT INTO chat_message (from_uid, to_uid, content) VALUES (?, ?, ?)"));
		pstmt->setInt(1, from_uid);
		pstmt->setInt(2, to_uid);
		pstmt->setString(3, content);
		pstmt->executeUpdate();

		std::unique_ptr<sql::Statement> stmt_last_id(con->_con->createStatement());
		std::unique_ptr<sql::ResultSet> res_id(stmt_last_id->executeQuery("SELECT LAST_INSERT_ID() AS id"));
		if (res_id->next()) {
			return res_id->getInt("id");
		}
		return -1;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return -1;
	}
}

std::vector<ChatMessage> MysqlDao::GetRecentMessages(int uid, int limit)
{
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return {};
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
	});

	std::vector<ChatMessage> messages;
	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"SELECT id, from_uid, to_uid, content, create_time FROM chat_message "
			"WHERE from_uid = ? OR to_uid = ? ORDER BY create_time ASC LIMIT ?"));
		pstmt->setInt(1, uid);
		pstmt->setInt(2, uid);
		pstmt->setInt(3, limit);

		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		while (res->next()) {
			ChatMessage msg;
			msg.id = res->getInt("id");
			msg.from_uid = res->getInt("from_uid");
			msg.to_uid = res->getInt("to_uid");
			msg.content = res->getString("content");
			msg.create_time = res->getString("create_time");
			messages.push_back(msg);
		}
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
	}
	return messages;
}
