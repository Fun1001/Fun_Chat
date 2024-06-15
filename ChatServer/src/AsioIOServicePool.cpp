#include "AsioIOServicePool.h"

AsioIOServicePool::~AsioIOServicePool() {
	Stop();
	std::cout << "AsioIOServicePool destruct!\n";
}

boost::asio::io_context& AsioIOServicePool::GetIOService() {
	auto& service = _ioServices[_nextIOService++];//�ȷ�����++
	if(_nextIOService == _ioServices.size())
	{
		_nextIOService = 0;
	}
	return service;
}

void AsioIOServicePool::Stop() {
	//����ִ��work.reset������io_context��run״̬�˳�
	//��iocontext�Ѿ��󶨶���д�ļ����¼�����Ҫ�ֶ�stop�ķ���
	for(auto& work:_works)
	{
		//������ֹͣ
		work->get_io_context().stop();
		work.reset();
	}
	//�̰߳�lambda���ʽ�����ܵĹ����У�ִ�������������terminate����
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
		//ʵ���̳߳أ�ʹ.run()�����أ�����io_context
		_works[i] = std::make_unique<Work>(_ioServices[i]);
		//_works[i] = std::unique_ptr<Work>(new Work(_ioServices[i]));
	}

	//����ioService �����̣߳��߳�������ioservice
	for (std::size_t i = 0; i < _ioServices.size(); ++i)
	{
		_threads.emplace_back([this, i]() {
			_ioServices[i].run();
			});
	}
}
