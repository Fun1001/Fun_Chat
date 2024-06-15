#include "CSession.h"

#include "CServer.h"
#include "LogicSystem.h"

CSession::CSession(boost::asio::io_context& io_context, CServer* server) :
	_socket(io_context), _server(server), _b_close(false), _b_head_parse(false){
	boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
	_uuid = boost::uuids::to_string(a_uuid);
	_recv_head_node = std::make_shared<MsgNode>(HEAD_TOTAL_LEN);
}

CSession::~CSession() {
	std::cout << "CSession destruct\n";
}

tcp::socket& CSession::GetSocket() {
	return _socket;
}

std::string& CSession::GetUuid() {
	return _uuid;
}

void CSession::Start() {
	AsyncReadHead(HEAD_TOTAL_LEN);
}

void CSession::Send(char* msg, short max_length, short msg_id) {
	std::lock_guard<std::mutex> lock(_send_lock);
	int send_que_size = _send_que.size();
	if(send_que_size > MAX_SENDQUE) {
		std::cout << "session: "<<_uuid<<" send queue is full, size is " << MAX_SENDQUE << std::endl;
		return;
	}
	_send_que.push(std::make_shared<SendNode>(msg, max_length, msg_id));
	//第一次发送size==0
	if(send_que_size > 0) {
		return;
	}

	auto& msgnode = _send_que.front();
	boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len), [this, self = SharedSelf()]
	(const boost::system::error_code& ec, std::size_t) {
		this->HandleWrite(ec, self);
	});
}

void CSession::Send(std::string msg, short msg_id) {
	std::lock_guard<std::mutex> lock(_send_lock);
	int send_que_size = _send_que.size();
	if (send_que_size > MAX_SENDQUE) {
		std::cout << "session: " << _uuid << " send queue is full, size is " << MAX_SENDQUE << std::endl;
		return;
	}
	//第一次发送size==0
	if (send_que_size > 0) {
		return;
	}

	_send_que.push(std::make_shared<SendNode>(msg.c_str(), msg.length(), msg_id));
	auto& msgnode = _send_que.front();
	boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len), [this, self = SharedSelf()]
	(const boost::system::error_code& ec, std::size_t) {
		this->HandleWrite(ec, self);
	});
}

void CSession::Close() {
	_socket.close();
	_b_close = true;
}

void CSession::AsyncReadBody(int total_len) {
	auto self = shared_from_this();
	asyncReadFull(total_len, [self, this, total_len](const boost::system::error_code& ec, std::size_t bytes_transfered) {
		try {
			if(ec) {
				std::cout << "handle read failed, error is " << ec.what() << std::endl;
				Close();
				_server->ClearSession(_uuid);
				return;
			}

			if(bytes_transfered < total_len) {
				std::cout << "read length not match, read [" << bytes_transfered << "], total [" << total_len << "]" << std::endl;
				Close();
				_server->ClearSession(_uuid);
				return;
			}

			memcpy(_recv_msg_node->_data, _data, bytes_transfered);
			_recv_msg_node->_cur_len += bytes_transfered;
			_recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';
			std::cout << "recv_msg is " << _recv_msg_node->_data << std::endl;

			//将消息投递到逻辑队列里
			LogicSystem::GetInstance()->PostMsgToQue(std::make_shared<LogicNode>(shared_from_this(), _recv_msg_node));

			//继续监听头部接受事件
			AsyncReadHead(HEAD_TOTAL_LEN);
		}
		catch (std::exception& e) {
			std::cout << "Exception code is " << e.what() << std::endl;
		}
	});
}

void CSession::AsyncReadHead(int total_len) {
	auto self = shared_from_this();//和其他管理Session的智能指针共享引用计数
	asyncReadFull(HEAD_TOTAL_LEN, [self, this](const boost::system::error_code& ec, std::size_t bytes_transfered) {
		try {
			if(ec) {
				std::cout << "handle read failed, error is " << ec.what() << std::endl;
				Close();
				_server->ClearSession(_uuid);
				return;
			}
			if(bytes_transfered < HEAD_TOTAL_LEN) {
				std::cout << "read length not match, read [" << bytes_transfered << "], total [" << HEAD_TOTAL_LEN << "]" << std::endl;
				Close();
				_server->ClearSession(_uuid);
				return;
			}
			_recv_head_node->Clear();
			memcpy(_recv_head_node->_data, _data, bytes_transfered);

			//获取头部Msg_id数据
			short msg_id = 0;
			memcpy(&msg_id, _recv_head_node->_data, HEAD_ID_LEN);
			//网络字节序转化为本地字节序
			msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
			std::cout << "msg_id is " << msg_id << std::endl;
			//id非法
			if(msg_id > MAX_LENGTH) {
				std::cout << "invalid msg_id is " << msg_id << std::endl;
				_server->ClearSession(_uuid);
				return;
			}

			short msg_len = 0;
			memcpy(&msg_len, _recv_head_node->_data + HEAD_ID_LEN, HEAD_DATA_LEN);
			//网络字节序转化为本地字节序
			msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);
			std::cout << "msg_len is " << msg_len << std::endl;
			//len非法
			if (msg_len > MAX_LENGTH) {
				std::cout << "invalid msg_len is " << msg_len << std::endl;
				_server->ClearSession(_uuid);
				return;
			}

			_recv_msg_node = std::make_shared<RecvNode>(msg_len, msg_id);
			AsyncReadBody(msg_len);
		}
		catch (std::exception& e) {
			std::cout << "Exception code is " << e.what() << std::endl;
		}
	});
}

std::shared_ptr<CSession> CSession::SharedSelf() {
	return shared_from_this();
}

void CSession::asyncReadFull(std::size_t max_len,
	std::function<void(const boost::system::error_code&, std::size_t)> handler) {
	//先清空要读的位置
	::memset(_data, 0, MAX_LENGTH);
	asyncReadLen(0, max_len, handler);
}

//读取指定字节数
void CSession::asyncReadLen(std::size_t read_len, std::size_t total_len,
	std::function<void(const boost::system::error_code&, std::size_t)> handler) {
	auto self = shared_from_this();
	_socket.async_read_some(boost::asio::buffer(_data + read_len, total_len - read_len), [read_len, total_len, handler, self]
		(const boost::system::error_code& ec, std::size_t bytesTransfered) {
		if(ec) {
			//出现错误
			handler(ec, read_len + bytesTransfered);
			return;
		}

		if(read_len + bytesTransfered >= total_len) {
			//长度足够，调用回调函数
			handler(ec, read_len + bytesTransfered);
			return;
		}

		self->asyncReadLen(read_len + bytesTransfered, total_len, handler);
	});
}

void CSession::HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self) {
	try {
		if(!error) {
			std::lock_guard<std::mutex> lock(_send_lock);
			_send_que.pop();
			if(!_send_que.empty()) {
				auto& msgnode = _send_que.front();
				boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len), [this, self = SharedSelf()]
				(const boost::system::error_code& ec, std::size_t) {
					this->HandleWrite(ec, self);
				});
			}
		}
		else {
			std::cout << "handle write failed, error is " << error.what() << std::endl;
			Close();
			_server->ClearSession(_uuid);
		}
	}
	catch (std::exception& e) {
		std::cout << "Exception code is " << e.what() << std::endl;
	}
}

LogicNode::LogicNode(shared_ptr<CSession> session, shared_ptr<RecvNode> recv_node) : _session(session), _recv_node(recv_node){
}
