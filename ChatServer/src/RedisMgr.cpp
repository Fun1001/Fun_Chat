#include "RedisMgr.h"
#include "ConfigMgr.h"

RedisMgr::~RedisMgr() {
	Close();
}

bool RedisMgr::Get(const std::string& key, std::string& value) {
	auto connect = con_pool_->getConnection();
	if(connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "Get %s", key.c_str());
	if (reply == NULL) {
		std::cout << "[Get " << key << "] Failed\n";
		freeReplyObject(reply);
		return false;
	}
	if (reply->type != REDIS_REPLY_STRING) {
		std::cout << "[Get " << key << "] Failed\n";
		freeReplyObject(reply);
		return false;
	}
	value = reply->str;
	freeReplyObject(reply);
	std::cout << "Success to execute command [ Get " << key << " ]\n";
	return true;
}

bool RedisMgr::Set(const std::string& key, const std::string& value) {
	auto connect = con_pool_->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "SET %s %s", key.c_str(), value.c_str());
	//如果返回NULL则说明执行失败
	if (reply == NULL) {
		std::cout << "[Set " << key << " " << value << "] Failed\n";
		freeReplyObject(reply);
		return false;
	}
	//如果执行失败则释放连接
	if (!(reply->type == REDIS_REPLY_STATUS && (strcmp(reply->str,"OK") == 0 || strcmp(reply->str, "ok") == 0))) {
		std::cout << "Excute Command [Set " << key << " " << value << "] Failed\n";
		freeReplyObject(reply);
		return false;
	}
	//执行成功 释放redisCommand执行后返回的redisReply所占用的内存
	freeReplyObject(reply);
	std::cout << "Success to execute command [SET " << key << " " << value << " ]\n";
	return true;
}

bool RedisMgr::Auth(const std::string& password) {
	auto connect = con_pool_->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "AUTH %s", password.c_str());
	if(reply->type == REDIS_REPLY_ERROR) {
		std::cout << "密码认证失败\n";
		freeReplyObject(reply);
		return false;
	}
	else {
		freeReplyObject(reply);
		std::cout << "密码认证成功\n";
		return true;
	}
}

bool RedisMgr::LPush(const std::string& key, const std::string& value) {
	auto connect = con_pool_->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "LPUSH %s %s", key.c_str(), value.c_str());
	if (reply == NULL) {
		std::cout << "[LPUSH " << key << " " << value << "] Failed\n";
		freeReplyObject(reply);
		return false;
	}
	if (reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0) {
		std::cout << "Excute Command [Set " << key << " " << value << "] Failed\n";
		freeReplyObject(reply);
		return false;
	}
	freeReplyObject(reply);
	std::cout << "Success to execute command [LPUSH " << key << " " << value << " ]\n";
	return true;
}

bool RedisMgr::LPop(const std::string& key, std::string& value) {
	auto connect = con_pool_->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "LPOP %s", key.c_str());

	if (reply->type != REDIS_REPLY_NIL || reply == nullptr) {
		std::cout << "Excute Command [LPOP " << key << "] Failed\n";
		freeReplyObject(reply);
		return false;
	}
	value = reply->str;
	freeReplyObject(reply);
	std::cout << "Success to execute command [LPUSH " << key << " ]\n";
	return true;
}

bool RedisMgr::RPush(const std::string& key, const std::string& value) {
	auto connect = con_pool_->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "RPUSH %s %s", key.c_str(), value.c_str());
	if (NULL == reply)
	{
		std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		return false;
	}

	if (reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0) {
		std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		return false;
	}

	std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] success ! " << std::endl;
	freeReplyObject(reply);
	return true;
}

bool RedisMgr::RPop(const std::string& key, std::string& value) {
	auto connect = con_pool_->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "RPOP %s ", key.c_str());
	if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
		std::cout << "Execut command [ RPOP " << key << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		return false;
	}
	value = reply->str;
	std::cout << "Execut command [ RPOP " << key << " ] success ! " << std::endl;
	freeReplyObject(reply);
	return true;
}

bool RedisMgr::HSet(const std::string& key, const std::string& hkey, const std::string& value) {
	auto connect = con_pool_->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "HSET %s %s %s", key.c_str(), hkey.c_str(), value.c_str());
	if(reply == nullptr || reply->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ HSET " << key << "  " << hkey << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		return false;
	}
	std::cout << "Execut command [ HSET " << key << "  " << hkey << "  " << value << " ] seccess ! " << std::endl;
	freeReplyObject(reply);
	return true;
}
//处理二进制数据
bool RedisMgr::HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen) {
	auto connect = con_pool_->getConnection();
	if (connect == nullptr) {
		return false;
	}
	const char* argv[4];
	size_t argvlen[4];
	argv[0] = "HSET";
	argvlen[0] = 4;
	argv[1] = key;
	argvlen[1] = strlen(key);
	argv[2] = hkey;
	argvlen[2] = strlen(hkey);
	argv[3] = hvalue;
	argvlen[3] = hvaluelen;
	
	auto reply = (redisReply*)redisCommandArgv(connect, 4, argv, argvlen);
	if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		return false;
	}
	std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] success ! " << std::endl;
	freeReplyObject(reply);
	return true;
}

std::string RedisMgr::HGet(const std::string& key, const std::string& hkey) {
	auto connect = con_pool_->getConnection();
	if (connect == nullptr) {
		return "";
	}
	const char* argv[3];
	size_t argvlen[3];
	argv[0] = "HGET";
	argvlen[0] = 4;
	argv[1] = key.c_str();
	argvlen[1] = key.length();
	argv[2] = hkey.c_str();
	argvlen[2] = hkey.length();
	
	auto reply = (redisReply*)redisCommandArgv(connect, 3, argv, argvlen);
	if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
		freeReplyObject(reply);
		std::cout << "Execut command [ HGet " << key << " " << hkey << "  ] failure ! " << std::endl;
		return "";
	}

	std::string value = reply->str;
	freeReplyObject(reply);
	std::cout << "Execut command [ HGet " << key << " " << hkey << " ] success ! " << std::endl;
	return value;
}

bool RedisMgr::Del(const std::string& key) {
	auto connect = con_pool_->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "DEL %s", key.c_str());
	if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ Del " << key << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		return false;
	}
	std::cout << "Execut command [ Del " << key << " ] success ! " << std::endl;
	freeReplyObject(reply);
	return true;
}

bool RedisMgr::ExistsKey(const std::string& key) {
	auto connect = con_pool_->getConnection();
	if (connect == nullptr) {
		return false;
	}
	auto reply = (redisReply*)redisCommand(connect, "exists %s", key.c_str());
	if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER || reply->integer == 0) {
		std::cout << "Not Found [ Key " << key << " ]  ! " << std::endl;
		freeReplyObject(reply);
		return false;
	}
	std::cout << " Found [ Key " << key << " ] exists ! " << std::endl;
	freeReplyObject(reply);
	return true;
}

void RedisMgr::Close() {
	con_pool_->Close();
}

RedisMgr::RedisMgr(){
	auto& gCfgMgr = ConfigMgr::Inst();
	auto host = gCfgMgr["Redis"]["Host"];
	auto port = gCfgMgr["Redis"]["Port"];
	auto pwd = gCfgMgr["Redis"]["Passwd"];
	con_pool_.reset(new RedisConPool(5, host.c_str(), atoi(port.c_str()), pwd.c_str()));
}

RedisConPool::RedisConPool(size_t poolSize, const char* host, int port, const char* pwd)
: poolSize_(poolSize), host_(host), port_(port), b_stop_(false){
	for(size_t i = 0; i<poolSize_;++i) {
		auto* context = redisConnect(host, port);
		if(context == NULL || context->err != 0) {
			if(context != nullptr) {
				redisFree(context);
			}
			continue;
		}

		auto reply = (redisReply*)redisCommand(context, "AUTH %s", pwd);
		if(reply->type == REDIS_REPLY_ERROR) {
			std::cout << "认证失败\n";
			freeReplyObject(reply);
			redisFree(context);
			continue;
		}
		freeReplyObject(reply);
		std::cout << "认证成功\n";
		connections_.push(context);
	}
}

RedisConPool::~RedisConPool() {
	std::lock_guard<std::mutex> lock(mutex_);
	while(!connections_.empty()) {
		connections_.pop();
	}
}

redisContext* RedisConPool::getConnection() {
	std::unique_lock<std::mutex> lock(mutex_);
	cond_.wait(lock, [this] {
		if(b_stop_) {
			return true;
		}
		return !connections_.empty();
		});
	if(b_stop_) {
		return nullptr;
	}
	auto* context = connections_.front();
	connections_.pop();
	return context;
}

void RedisConPool::returnConnection(redisContext* context) {
	std::lock_guard<std::mutex> lock(mutex_);
	if(b_stop_) {
		return;
	}
	connections_.push(context);
	cond_.notify_one();
}

void RedisConPool::Close() {
	b_stop_ = true;
	cond_.notify_all();
}
