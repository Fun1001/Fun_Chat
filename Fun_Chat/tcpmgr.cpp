#include "tcpmgr.h"
#include <QAbstractSocket>
#include <QJsonDocument>
#include <usermgr.h>
TcpMgr::~TcpMgr()
{

}

TcpMgr::TcpMgr():_host(""), _port(0), _b_recv_pending(false), _message_id(0), _message_len(0)
{
    QObject::connect(&_socket, &QTcpSocket::connected,[&](){
        qDebug()<<"Connected to Server!";
        emit sig_con_success(true);
    });
    QObject::connect(&_socket, &QTcpSocket::readyRead, [&](){
        //当有数据可读时，读取所有数据并追加到缓冲区
        _buffer.append(_socket.readAll());

        QDataStream stream(&_buffer,QIODevice::ReadOnly);
        stream.setVersion(QDataStream::Qt_5_0);

        forever{
            if(!_b_recv_pending){
                //检查缓冲区的数据是否足够解析出一个消息头（id+len）
                if(_buffer.size() < static_cast<int>(sizeof (quint16) * 2)){
                    return;
                }
                //预读id和len但不从缓冲区移除
                stream >> _message_id >> _message_len;
                //将buffer中前4个字节移除
                _buffer = _buffer.mid(sizeof (quint16) * 2);

                //输出读取的数据
                qDebug()<<"message id:"<<_message_id<<"message len:"<<_message_len;
            }
            if(_buffer.size() < _message_len){
                _b_recv_pending = true;
                return;
            }
            _b_recv_pending = false;

            //读取消息体
            QByteArray messageBody = _buffer.mid(0, _message_len);
            qDebug()<<"mesaage is:"<<messageBody;

            _buffer = _buffer.mid(_message_len);
            handleMsg(ReqId(_message_id), _message_len, messageBody);
        }
    });

    //错误处理
    QObject::connect(&_socket, static_cast<void (QTcpSocket::*)(QTcpSocket::SocketError)>(&QTcpSocket::error),
                     [&](QTcpSocket::SocketError socketError){
        qDebug()<<"Error: "<<_socket.errorString();
        switch (socketError) {
            case QTcpSocket::ConnectionRefusedError:
                qDebug()<<"Connection Refused!";
                emit sig_con_success(false);
                break;
            case QTcpSocket::RemoteHostClosedError:
                qDebug()<<"Remote Host Closed Connection!";
                emit sig_con_success(false);
                break;
            case QTcpSocket::HostNotFoundError:
                qDebug()<<"Host Not Found!";
                emit sig_con_success(false);
                break;
            case QTcpSocket::SocketTimeoutError:
                qDebug()<<"Connection Timeout!";
                emit sig_con_success(false);
                break;
            case QTcpSocket::NetworkError:
                qDebug()<<"Network Error!";
                emit sig_con_success(false);
                break;
            default:
                qDebug()<<"Other Error!";
                break;
        }
    });

    //处理断开连接
    QObject::connect(&_socket, &QTcpSocket::disconnected, [&](){
        qDebug()<<"Disconnected from server...";
    });

    //连接发送信号 发送数据
    QObject::connect(this, &TcpMgr::sig_send_data, this, &TcpMgr::slot_send_data);

    //注册消息
    initHandlers();
}

void TcpMgr::initHandlers()
{
    //auto self = shared_from_this() 不适用，因为shared_from_this要求类要构造完才可以使用，而initHandlers在构造中使用
    _handlers.insert(ID_CHAT_LOGIN_RSP, [this](ReqId id, int len, QByteArray data){
        Q_UNUSED(len);
        qDebug()<<"handle id is "<<id<<" data is "<<data;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

        if(jsonDoc.isNull()){
            qDebug()<<"Failed to create QJsonDocument.";
            return;
        }

        QJsonObject jsonObj = jsonDoc.object();

        if(!jsonObj.contains("error")){
            int err = ErrorCodes::ERR_JSON;
            qDebug()<<"Login Failed, err is Json Parse Err "<<err;
            emit sig_login_failed(err);
            return;
        }

        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug()<<"Login Failed, err is "<<err;
            emit sig_login_failed(err);
            return;
        }

        UserMgr::GetInstance()->SetUid(jsonObj["uid"].toInt());
        UserMgr::GetInstance()->SetName(jsonObj["name"].toString());
        UserMgr::GetInstance()->SetToken(jsonObj["token"].toString());

        emit sig_switch_chatdlg();
    });
}

void TcpMgr::handleMsg(ReqId id, int len, QByteArray data)
{
    auto find_iter = _handlers.find(id);
    if(find_iter == _handlers.end()){
        qDebug()<<"not found id ["<<id<<"] to handle";
        return;
    }

    find_iter.value()(id,len,data);
}

void TcpMgr::slot_tcp_connect(ServerInfo si)
{
    qDebug()<<"recevice tcp connect signal";
    //连接到服务器
    qDebug()<<"Connectint to server...";
    _host = si.Host;
    _port = static_cast<uint16_t>(si.Port.toUInt());
    _socket.connectToHost(si.Host, _port);
}

void TcpMgr::slot_send_data(ReqId reqId, QString data)
{
    uint16_t id = reqId;
    //将字符串转换为UTF-8字符数组
    QByteArray dataBytes = data.toUtf8();

    //计算长度（使用网络字节序转换）
    quint16 len = static_cast<quint16>(dataBytes.size());

    //创建一个QByteArray放要发送的所有数据
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);

    //设置字节流使用网络字节序
    out.setByteOrder(QDataStream::BigEndian);

    out<<id<<len;
    //添加数据
    block.append(dataBytes);
    //发送数据
    _socket.write(block);
}
