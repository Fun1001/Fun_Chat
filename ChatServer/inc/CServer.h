#pragma once
#include "const.h"
#include <boost/asio.hpp>
#include "CSession.h"
class CServer :public std::enable_shared_from_this<CServer>
{
public:
    CServer(boost::asio::io_context& ioc, unsigned short& port);
    ~CServer();
    void ClearSession(std::string);
private:
    //���ܶԶ�����
    void HandleAccept(std::shared_ptr<CSession>, const boost::system::error_code& error);
    //��ʼ��������
    void StartAccept();

    unsigned short _port;
    tcp::acceptor  _acceptor;
    net::io_context& _ioc;
    std::mutex _mutex;
    std::map<std::string, std::shared_ptr<CSession>> _sessions;
};