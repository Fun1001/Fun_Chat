#pragma once
#include "const.h"
#include "Singleton.h"
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "message.pb.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginReq;
using message::LoginRsp;
using message::StatusService;

class StatusConPool
{
public:
	StatusConPool(size_t poolSize, std::string host, std::string port);
	~StatusConPool();
	void Close();
	std::unique_ptr<StatusService::Stub> getConnection();
	void returnConnection(std::unique_ptr<StatusService::Stub> context);
private:
	std::atomic<bool> b_stop_;
	size_t poolSize_;
	std::string host_;
	std::string port_;
	//使用队列管理
	std::queue<std::unique_ptr<StatusService::Stub>> connections_;
	std::condition_variable cond_;
	std::mutex mutex_;
};

class StatusGrpcClient :public Singleton<StatusGrpcClient>
{
	friend class Singleton<StatusGrpcClient>;
public:
	~StatusGrpcClient() {
		
	}
	GetChatServerRsp GetChatServer(int uid);
	LoginRsp Login(int uid, std::string token);
private:
	StatusGrpcClient();
	std::unique_ptr<StatusConPool> pool_;
};
