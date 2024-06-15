#pragma once
#include "const.h"
#include "CSession.h"
#include <map>
#include <queue>
#include <thread>
#include <functional>
#include <unordered_map>

//定义一个函数的处理器
typedef std::function<void(std::shared_ptr<CSession>, const short& msg_id, const string& msg_data)> FunCallBack;
class LogicSystem: public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem();
	void PostMsgToQue(std::shared_ptr<LogicNode> msg);
private:
	LogicSystem();
	void DealMsg();
	void RegisterCallBacks();
	void LoginHandler(std::shared_ptr<CSession>, const short& msg_id, const string& msg_data);

	std::thread _work_thread;
	std::queue<std::shared_ptr<LogicNode>> _msg_que;
	std::mutex _mutex;
	std::condition_variable _consume;
	std::condition_variable _produce;
	bool _b_stop;
	std::map<short, FunCallBack> _fun_callbacks;
	std::unordered_map<int, std::shared_ptr<UserInfo>> _users;

	const size_t _max_queue_size = 100; // 设置队列的最大长度
};

