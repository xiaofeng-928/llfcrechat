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
		// 简化版：只查询uid, name, pwd
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
