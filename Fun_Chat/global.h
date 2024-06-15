 #ifndef GLOBAL_H
#define GLOBAL_H
#include <QWidget>
#include <functional>
#include <QRegularExpression>
#include "QStyle"
#include <memory>
#include <iostream>
#include <mutex>
#include <QByteArray>
#include <QNetworkReply>
#include <QJsonObject>
#include <QDir>
#include <QSettings>
/******************************************************************************
 * @brief      repolish 用来刷新qss
 *****************************************************************************/
extern std::function<void(QWidget*)> repolish;
extern std::function<QString(QString)> xorString;

enum ReqId{
    ID_GET_VERIFY_CODE = 1001,//获取验证码
    ID_REG_USER = 1002, //注册用户
    ID_RESET_PWD = 1003, //修改密码
    ID_LOGIN_USER = 1004, //用户登录
    ID_CHAT_LOGIN = 1005, //登录聊天服务器
    ID_CHAT_LOGIN_RSP = 1006, //登录聊天服务器回包
};

enum Modules{
    REGISTERMOD = 0,//注册模块
    RESETMOD = 1,//重置模块
    LOGINMOD = 2,//登录模块
};

enum ErrorCodes{
    SUCCESS = 0,
    ERR_JSON = 1,//json解释失败
    ERR_NETWORK = 2,//网络错误
};

enum TipErr{
    TIP_SUCCESS = 0,
    TIP_EMAIL_ERR = 1,
    TIP_PWD_ERR = 2,
    TIP_CONFIRM_ERR = 3,
    TIP_PWD_CONFIRM = 4,
    TIP_VERIFY_ERR = 5,
    TIP_USER_ERR = 6
};
//包含两种状态一个是普通状态，一个是选中状态。而Label中的六种状态就是基于这两种状态嵌套实现的。
enum ClickLbState{
    Normal = 0,
    Selected = 1
};

extern QString gate_url_prefix;

struct ServerInfo{
    QString Host;
    QString Port;
    QString Token;
    int Uid;
};

#endif // GLOBAL_H
