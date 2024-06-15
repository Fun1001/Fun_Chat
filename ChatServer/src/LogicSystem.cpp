#include "LogicSystem.h"
#include "StatusGrpcClient.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"


LogicSystem::~LogicSystem() {
	_b_stop = true;
	_consume.notify_one();
	_work_thread.join();
}

//生产者
void LogicSystem::PostMsgToQue(std::shared_ptr<LogicNode> msg) {
	std::unique_lock<std::mutex> unique_lk(_mutex);

	_produce.wait(unique_lk, [this]() {
		return _msg_que.size() < _max_queue_size;
		});

	_msg_que.push(msg);

	//由0变1则发送通知信号
	if(_msg_que.size() == 1) {
		unique_lk.unlock();
		_consume.notify_one();
	}
}

LogicSystem::LogicSystem() : _b_stop(false) {
	RegisterCallBacks();
	_work_thread = std::thread(&LogicSystem::DealMsg, this);
}

//消费者
void LogicSystem::DealMsg() {
	for(;;) {
		std::unique_lock<std::mutex> unique_lk(_mutex);
		//如果队列为空或者关闭，阻塞
		while(_msg_que.empty() && !_b_stop) {
			_consume.wait(unique_lk);
		}

		//判断是否是关闭状态，把所有逻辑执行完毕后退出循环
		if(_b_stop) {
			while (!_msg_que.empty()) {
				auto msg_node = _msg_que.front();
				std::cout << "close---, recv_msg id is " << msg_node->_recv_node->_msg_id << std::endl;
				auto callback_iter = _fun_callbacks.find(msg_node->_recv_node->_msg_id);
				if(callback_iter == _fun_callbacks.end()) {
					_msg_que.pop();
					continue;
				}
				callback_iter->second(msg_node->_session, msg_node->_recv_node->_msg_id, std::string(msg_node->_recv_node->_data, msg_node->_recv_node->_cur_len));
				_msg_que.pop();
			}
			break;
		}

		//没有停止，处理队列中的数据
		auto msg_node = _msg_que.front();
		std::cout << "recv_msg id is " << msg_node->_recv_node->_msg_id << std::endl;
		auto callback_iter = _fun_callbacks.find(msg_node->_recv_node->_msg_id);
		if (callback_iter == _fun_callbacks.end()) {
			_msg_que.pop();
			std::cout << "msg id [" << msg_node->_recv_node->_msg_id << "] handler not found" << std::endl;
			continue;
		}
		callback_iter->second(msg_node->_session, msg_node->_recv_node->_msg_id, std::string(msg_node->_recv_node->_data, msg_node->_recv_node->_cur_len));
		_msg_que.pop();

		_produce.notify_one();
	}
}

void LogicSystem::RegisterCallBacks() {
	_fun_callbacks[MSG_CHAT_LOGIN] = [this](std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
		this->LoginHandler(session, msg_id, msg_data);
	};
}

void LogicSystem::LoginHandler(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data, root);
	auto uid = root["uid"].asInt();
	std::cout << "user login uid is " << uid << " token is " << root["token"].asString() << std::endl;

	//从状态服务器获取token,匹配是否一致
	auto rsp = StatusGrpcClient::GetInstance()->Login(uid, root["token"].asString());
	Json::Value rtvalue;

	Defer defer([this, &rtvalue, session]() {
		std::string return_str = rtvalue.toStyledString();
		session->Send(return_str, MSG_CHAT_LOGIN_RSP);
	});

	rtvalue["error"] = rsp.error();
	if(rsp.error() != ErrorCodes::Success) {
		return;
	}

	//在内存中查询用户信息
	auto find_iter = _users.find(uid);
	std::shared_ptr<UserInfo> user_info = nullptr;
	if(find_iter == _users.end()) {
		//数据库查询
		user_info = MysqlMgr::GetInstance()->GetUser(uid);
		if(user_info == nullptr) {
			rtvalue["error"] = ErrorCodes::UidInvalid;
			return;
		}

		_users[uid] = user_info;
	}
	else {
		user_info = find_iter->second;
	}

	rtvalue["uid"] = uid;
	rtvalue["token"] = rsp.token();
	rtvalue["name"] = user_info->name;
}
