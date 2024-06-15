#include "AsioIOServicePool.h"

AsioIOServicePool::~AsioIOServicePool() {
	Stop();
	std::cout << "AsioIOServicePool destruct!\n";
}

boost::asio::io_context& AsioIOServicePool::GetIOService() {
	auto& service = _ioServices[_nextIOService++];//先返回再++
	if(_nextIOService == _ioServices.size())
	{
		_nextIOService = 0;
	}
	return service;
}

void AsioIOServicePool::Stop() {
	//仅仅执行work.reset不能让io_context从run状态退出
	//当iocontext已经绑定读或写的监听事件后，需要手动stop改服务
	for(auto& work:_works)
	{
		//服务先停止
		work->get_io_context().stop();
		work.reset();
	}
	//线程绑定lambda表达式，再跑的过程中，执行析构，会出现terminate错误
	for(auto& t: _threads)
	{
		t.join();
	}
}

AsioIOServicePool::AsioIOServicePool(std::size_t size):_ioServices(size), _works(size), _nextIOService(0)
{
	std::cout << "create " << size << " AsioIOService\n";
	for(std::size_t i =0; i<size; ++i)
	{
		//实现线程池，使.run()不返回，监听io_context
		_works[i] = std::make_unique<Work>(_ioServices[i]);
		//_works[i] = std::unique_ptr<Work>(new Work(_ioServices[i]));
	}

	//遍历ioService 创建线程，线程内启动ioservice
	for (std::size_t i = 0; i < _ioServices.size(); ++i)
	{
		_threads.emplace_back([this, i]() {
			_ioServices[i].run();
			});
	}
}
