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
    //接受对端连接
    void HandleAccept(std::shared_ptr<CSession>, const boost::system::error_code& error);
    //开始接受连接
    void StartAccept();

    unsigned short _port;
    tcp::acceptor  _acceptor;
    net::io_context& _ioc;
    std::mutex _mutex;
    std::map<std::string, std::shared_ptr<CSession>> _sessions;
};