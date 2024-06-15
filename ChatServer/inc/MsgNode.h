#pragma once
#include <iostream>
#include <string>
#include "const.h"
#include "boost/asio.hpp"
class LogicSystem;

class MsgNode
{
public:
	explicit MsgNode(const short max_len);
	~MsgNode();
	void Clear();

	short _cur_len;
	short _total_len;
	char* _data;
};

class RecvNode:public MsgNode
{
	friend class LogicSystem;
public:
	RecvNode(short max_len, short msg_id);
private:
	short _msg_id;
};

class SendNode:public MsgNode
{
	friend class LogicSystem;
public:
	SendNode(const char* msg, short max_len, short msg_id);
private:
	short _msg_id;
};

