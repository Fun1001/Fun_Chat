#include "CServer.h"
#include "AsioIOServicePool.h"
CServer::CServer(boost::asio::io_context& ioc, unsigned short& port) :_ioc(ioc), _port(port),
_acceptor(ioc, tcp::endpoint(tcp::v4(), port)) {
    std::cout << "Server start success, listening on port: " << _port << std::endl;
    StartAccept();
}

CServer::~CServer()
{
}

void CServer::ClearSession(std::string uuid) {
    std::lock_guard<mutex> lock(_mutex);
    _sessions.erase(uuid);
}

void CServer::HandleAccept(shared_ptr<CSession> new_session, const boost::system::error_code& error) {
    if(!error) {
        new_session->Start();
        std::lock_guard<std::mutex> lock(_mutex);
        _sessions.insert(make_pair(new_session->GetUuid(), new_session));
    }
    else {
        std::cout << "session accept failed, error is " << error.what() << std::endl;
    }
}

void CServer::StartAccept() {
    auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
    std::shared_ptr<CSession> new_session = std::make_shared<CSession>(io_context, this);
    _acceptor.async_accept(new_session->GetSocket(), [this, new_session](const boost::system::error_code& error) {
        HandleAccept(new_session, error);
    });
}
