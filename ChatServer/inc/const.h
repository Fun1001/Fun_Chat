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
	Error_Json = 1001,//json解析错误
	RPCFailed = 1002,//RPC请求错误
	VerifyExpired = 1003,//验证码过期
	VerifyCodeErr = 1004,//验证码错误
	UserExist = 1005,//用户已存在 
	PasswdErr = 1006,//密码错误
	EmailNotMatch = 1007,//邮箱不匹配
	PasswdUpFailed = 1008,//更新密码失败
	PasswdInvalid = 1009,//密码更新失败
	TokenInvalid = 1010,   //Token失效
	UidInvalid = 1011,  //uid无效
};

#define CODEPREFIX "code_"

//RAII技术 类似Go中的defer
class Defer
{
public:
	//接受一个lambda表达式或者函数指针
	Defer(std::function<void()> func): _func(func){}

	//析构函数中执行传入的函数
	~Defer() {
		_func();
	}

private:
	std::function<void()> _func;
};

#define MAX_LENGTH  1024*2
//头部总长度
#define HEAD_TOTAL_LEN 4
//头部id长度
#define HEAD_ID_LEN 2
//头部数据长度
#define HEAD_DATA_LEN 2
#define MAX_RECVQUE  10000
#define MAX_SENDQUE 1000

enum MSG_IDS {
	MSG_CHAT_LOGIN = 1005, //用户登陆
	MSG_CHAT_LOGIN_RSP = 1006, //用户登陆回包
};