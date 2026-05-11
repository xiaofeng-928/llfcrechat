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

int MysqlDao::RegUser(const std::string& name, const std::string& pwd)
{
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return -1;
	}

	Defer defer([this, &con] {
		pool_->returnConnection(std::move(con));
	});

	try {
		con->_con->setAutoCommit(false);

		// 检查用户名是否已存在

		std::unique_ptr<sql::PreparedStatement> pstmt_user(con->_con->prepareStatement("SELECT 1 FROM user WHERE name = ?"));
		pstmt_user->setString(1, name);
		std::unique_ptr<sql::ResultSet> res_user(pstmt_user->executeQuery());
		if (res_user->next()) {
			con->_con->rollback();
			std::cout << "name " << name << " already exists" << std::endl;
			return 0;
		}

		// 更新用户ID
		std::unique_ptr<sql::PreparedStatement> pstmt_upid(con->_con->prepareStatement("UPDATE user_id SET id = id + 1"));
		pstmt_upid->executeUpdate();

		// 获取新ID
		std::unique_ptr<sql::PreparedStatement> pstmt_uid(con->_con->prepareStatement("SELECT id FROM user_id"));
		std::unique_ptr<sql::ResultSet> res_uid(pstmt_uid->executeQuery());
		int newId = 0;
		if (res_uid->next()) {
			newId = res_uid->getInt("id");
		} else {
			con->_con->rollback();
			std::cout << "select id from user_id failed" << std::endl;
			return -1;
		}

		// 插入用户：只有uid, name, pwd
		std::unique_ptr<sql::PreparedStatement> pstmt_insert(con->_con->prepareStatement(
			"INSERT INTO user (uid, name, pwd) VALUES (?, ?, ?)"));
		pstmt_insert->setInt(1, newId);
		pstmt_insert->setString(2, name);
		pstmt_insert->setString(3, pwd);
		pstmt_insert->executeUpdate();

		con->_con->commit();
		std::cout << "new user insert success, uid=" << newId << std::endl;
		return newId;
	}
	catch (sql::SQLException& e) {
		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return -1;
	}
}

bool MysqlDao::CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
	});

	try {
		// 通过用户名查询用户

		std::unique_ptr<sql::PreparedStatement> pstmt_user(con->_con->prepareStatement("SELECT uid, name, pwd FROM  user WHERE name = ?"));
		pstmt_user->setString(1, name);
		std::unique_ptr<sql::ResultSet> res_user(pstmt_user->executeQuery());

		std::string origin_pwd = "";

		if (res_user->next()) {
			origin_pwd = res_user->getString("pwd");
			std::cout << "Stored password: " << origin_pwd << std::endl;

			if (pwd != origin_pwd) {
				return false;
			}
			userInfo.uid = res_user->getInt("uid");
			userInfo.name = res_user->getString("name");
			userInfo.pwd = origin_pwd;
			return true;
		}
		return false;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
	return true;
}
