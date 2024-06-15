#include "MysqlDao.h"
#include "ConfigMgr.h"

MySqlPool::MySqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize)
: url_(url), user_(user), pass_(pass), schema_(schema), poolSize_(poolSize), b_stop_(false){
	try {
		for(int i = 0; i<poolSize_; ++i) {
			sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
			auto* con = driver->connect(url_, user_, pass_);
			con->setSchema(schema_);
			//获取当前时间戳
			auto currentTime = std::chrono::system_clock::now().time_since_epoch();
			//转换为秒
			long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
			pool_.push(std::make_unique<SqlConnection>(con, timestamp));
		}
		_check_thread = std::thread([this]() {
			while (!b_stop_) {
				checkConnection();
				std::this_thread::sleep_for(std::chrono::seconds(60));
			}
			});
		_check_thread.detach();
	}
	catch (sql::SQLException& e) {
		std::cout << "mysql pool init failed ,error is " << e.what() << std::endl;
	}
}

void MySqlPool::checkConnection() {
	std::lock_guard<std::mutex> lock(mutex_);
	//遍历队列，更新时间
	int poolsize = pool_.size();
	//获取当前时间戳
	auto currentTime = std::chrono::system_clock::now().time_since_epoch();
	//转换为秒
	long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
	for(int i = 0; i < poolsize; ++i) {
		auto con = std::move(pool_.front());
		pool_.pop();

		//RAII 技术 类似 Go中的Defer
		Defer defer([this, &con]() {
			pool_.push(std::move(con));
		});

		if(timestamp - con->_last_oper_time < 5) {
			continue;
		}
		//更新连接
		try {
			std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
			stmt->executeQuery("SELECT 1");
			con->_last_oper_time = timestamp;
			std::cout << "execute timer alive query ,cur is " << timestamp << std::endl;
		}
		catch (sql::SQLException& e) {
			std::cout << "Error keeping connection alive: " << e.what() << std::endl;
			//创建新的连接 并替换
			sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
			auto* newcon = driver->connect(url_, user_, pass_);
			newcon->setSchema(schema_);
			con->_con.reset(newcon);
			con->_last_oper_time = timestamp;
		}
	}
}

std::unique_ptr<SqlConnection> MySqlPool::getConnection() {
	std::unique_lock<std::mutex> lock(mutex_);
	//返回false条件变量挂起，线程等待
	cond_.wait(lock, [this] {
		if(b_stop_) {
			return true;
		}
		return !pool_.empty();
		});
	if(b_stop_) {
		return nullptr;
	}
	std::unique_ptr<SqlConnection> con(std::move(pool_.front()));
	pool_.pop();
	return con;
}

void MySqlPool::returnConnection(std::unique_ptr<SqlConnection> con) {
	std::unique_lock<std::mutex> lock(mutex_);
	if(b_stop_) {
		return;
	}
	//是std::unique_ptr，是独占所有权的智能指针，不能直接复制
	pool_.push(std::move(con));
	cond_.notify_one();
}

void MySqlPool::Close() {
	b_stop_ = true;
	cond_.notify_all();
}

MySqlPool::~MySqlPool() {
	std::unique_lock<std::mutex> lock(mutex_);
	while(!pool_.empty()) {
		pool_.pop();
	} 
}

MysqlDao::MysqlDao() {
	auto& cfg = ConfigMgr::Inst();
	const auto& host = cfg["Mysql"]["Host"];
	const auto& port = cfg["Mysql"]["Port"];
	const auto& pwd = cfg["Mysql"]["Passwd"];
	const auto& schema = cfg["Mysql"]["Schema"];
	const auto& user = cfg["Mysql"]["User"];
	pool_.reset(new MySqlPool(host + ":" + port, user, pwd, schema, 5));
}

MysqlDao::~MysqlDao() {
	pool_->Close();
}

int MysqlDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd) {
	auto con = pool_->getConnection();
	try {
		if(con == nullptr) {
			return false;
		}
		//准备调用存储过程
		std::unique_ptr<sql::PreparedStatement> stmt(con->_con->prepareStatement("CALL reg_user(?,?,?,@result)"));
		//设置输入参数
		stmt->setString(1, name);
		stmt->setString(2, email);
		stmt->setString(3, pwd);

		//由于PreparedStatement不直接支持注册输出参数，需要使用会话变量或其他方法获取输出参数

		//执行存储过程
		stmt->execute();
		//如果存储过程设置了会话变量或有其他方式获取输出参数的值，可以使用SELECT查询获取
		//例如，@result可以这样获取
		std::unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
		std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));
		if(res->next()) {
			int result = res->getInt("result");
			std::cout << "Result is: " << result << std::endl;
			pool_->returnConnection(std::move(con));
			return result;
		}
		pool_->returnConnection(std::move(con));
		return -1;
	}
	catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << "(mysql errorcode: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << std::endl;
		return -1;
	}
}

bool MysqlDao::CheckEmail(const std::string& name, const std::string& email) {
	auto con = pool_->getConnection();
	try {
		if(con == nullptr) {
			return false;
		}
		//准备查询语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT email FROM user WHERE name = ?"));

		//绑定参数
		pstmt->setString(1, name);

		//执行查询
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		//遍历结果集
		while (res->next()) {
			std::cout << "Check Email: " << res->getString("email") << std::endl;
			if(email != res->getString("email")) {
				pool_->returnConnection(std::move(con));
				return false;
			}
			pool_->returnConnection(std::move(con));
			return true;
		}
		return true;
	}
	catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << "(mysql errorcode: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << std::endl;
		return false;
	}
}

bool MysqlDao::UpdatePwd(const std::string& name, const std::string& newpwd) {
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			return false;
		}
		//准备查询语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));

		//绑定参数
		pstmt->setString(1, newpwd);
		pstmt->setString(2, name);

		//执行查询
		int updateCount = pstmt->executeUpdate();

		std::cout << "update rows: " << updateCount << std::endl;
		pool_->returnConnection(std::move(con));
		return true;
	}
	catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << "(mysql errorcode: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << std::endl;
		return false;
	}
}

bool MysqlDao::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
	});

	try {

		//准备查询语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE email = ?"));

		//绑定参数
		pstmt->setString(1, email);
		std::string origin_pwd = "";

		//执行查询
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		//遍历结果集
		while (res->next()) {
			origin_pwd = res->getString("pwd");
			std::cout << "PWD: " << origin_pwd << std::endl;
			break;
		}
		if(pwd != origin_pwd) {
			return false;
		}
		userInfo.name = res->getString("name");
		userInfo.email = email;
		userInfo.uid = res->getInt("uid");
		userInfo.pwd = origin_pwd;
		return true;
	}
	catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << "(mysql errorcode: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << std::endl;
		return false;
	}
}

std::shared_ptr<UserInfo> MysqlDao::GetUser(int uid) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return nullptr;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		//准备查询语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE uid = ?"));

		//绑定参数
		pstmt->setInt(1, uid);

		//执行查询
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		std::shared_ptr<UserInfo> user_ptr = nullptr;
		//遍历结果集
		while (res->next()) {
			user_ptr.reset(new UserInfo);
			user_ptr->pwd = res->getString("pwd");
			user_ptr->email = res->getString("email");
			user_ptr->name = res->getString("name");
			user_ptr->uid = uid;
			break;
		}

		return user_ptr;
	}
	catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << "(mysql errorcode: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << std::endl;
		return nullptr;
	}
}
