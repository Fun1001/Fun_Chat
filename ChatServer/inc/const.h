#pragma once
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <memory>
#include <iostream>
#include <mutex>
#include <functional>
#include <map>
#include <unordered_map>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <cassert>

#include "hiredis.h"
#include "Singleton.h"
#include "data.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

enum ErrorCodes {
	Success = 0,
	Error_Json = 1001,//json��������
	RPCFailed = 1002,//RPC�������
	VerifyExpired = 1003,//��֤�����
	VerifyCodeErr = 1004,//��֤�����
	UserExist = 1005,//�û��Ѵ��� 
	PasswdErr = 1006,//�������
	EmailNotMatch = 1007,//���䲻ƥ��
	PasswdUpFailed = 1008,//��������ʧ��
	PasswdInvalid = 1009,//�������ʧ��
	TokenInvalid = 1010,   //TokenʧЧ
	UidInvalid = 1011,  //uid��Ч
};

#define CODEPREFIX "code_"

//RAII���� ����Go�е�defer
class Defer
{
public:
	//����һ��lambda����ʽ���ߺ���ָ��
	Defer(std::function<void()> func): _func(func){}

	//����������ִ�д���ĺ���
	~Defer() {
		_func();
	}

private:
	std::function<void()> _func;
};

#define MAX_LENGTH  1024*2
//ͷ���ܳ���
#define HEAD_TOTAL_LEN 4
//ͷ��id����
#define HEAD_ID_LEN 2
//ͷ�����ݳ���
#define HEAD_DATA_LEN 2
#define MAX_RECVQUE  10000
#define MAX_SENDQUE 1000

enum MSG_IDS {
	MSG_CHAT_LOGIN = 1005, //�û���½
	MSG_CHAT_LOGIN_RSP = 1006, //�û���½�ذ�
};