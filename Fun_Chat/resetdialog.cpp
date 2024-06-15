#include "resetdialog.h"
#include "ui_resetdialog.h"
#include <QDebug>
#include <QRegularExpression>
#include "global.h"
#include "httpmgr.h"

ResetDialog::ResetDialog(QWidget *parent):
    QDialog(parent),
    ui(new Ui::ResetDialog)
{
    ui->setupUi(this);
    ui->pwd_edit->setEchoMode(QLineEdit::Password);
    connect(ui->user_edit, &QLineEdit::editingFinished, this, [this](){
        checkUserValid();
    });
    connect(ui->email_edit, &QLineEdit::editingFinished, this, [this](){
        checkEmailValid();
    });
    connect(ui->pwd_edit, &QLineEdit::editingFinished, this, [this](){
        checkPassValid();
    });

    //连接reset相关信号和注册处理回调
    initHttpHandlers();
    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_reset_mod_finish, this, &ResetDialog::slot_reset_mod_finish);
}

ResetDialog::~ResetDialog()
{
    delete ui;
}

void ResetDialog::initHttpHandlers()
{
    //获取验证码回包逻辑
    _handlers.insert(ReqId::ID_GET_VERIFY_CODE, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"), false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("验证码已发送，请注意查收"), true);
        qDebug() << "email is" << email;
    });
    //修改用户密码回包逻辑
    _handlers.insert(ReqId::ID_RESET_PWD, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"), false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("重置密码成功，点击返回登录界面"), true);
        qDebug() << "email is" << email;
        qDebug() << "user uuid is" << jsonObj["uuid"].toString();
    });
}

void ResetDialog::showTip(QString str, bool b_ok)
{
    if(b_ok){
        ui->err_tip->setProperty("state", "normal");
    }else{
        ui->err_tip->setProperty("state", "err");
    }
    ui->err_tip->setText(str);
    repolish(ui->err_tip);
}

void ResetDialog::AddTipErr(TipErr te, QString tips)
{
    _tip_errs[te] = tips;
    showTip(tips,false);
}

void ResetDialog::DelTipErr(TipErr te)
{
    _tip_errs.remove(te);
    if(_tip_errs.empty()){
        ui->err_tip->clear();
        return;
    }

    showTip(_tip_errs.first(),false);
}

bool ResetDialog::checkUserValid()
{
    if(ui->user_edit->text() == ""){
        AddTipErr(TipErr::TIP_USER_ERR,"用户名不能为空");
        return false;
    }
    DelTipErr(TipErr::TIP_USER_ERR);
    return true;
}

bool ResetDialog::checkEmailValid()
{
    auto email = ui->email_edit->text();
    QRegularExpression regex(R"((\w+)(\. | _)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch();
    if(!match){
        AddTipErr(TipErr::TIP_EMAIL_ERR,"邮箱地址不正确");
        return false;
    }
    DelTipErr(TipErr::TIP_EMAIL_ERR);
    return true;
}

bool ResetDialog::checkPassValid()
{
    auto pass = ui->pwd_edit->text();
    if(pass.length() < 6 || pass.length() > 15){
        AddTipErr(TipErr::TIP_PWD_ERR, tr("密码长度应为6-15"));
        return false;
    }
    //正则解释
    // ^[a-zA-Z0-9!@#$%^&*]{6,15}$ 密码长度至少6，可以是字母、数字和特定的特殊字符
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*]{6,15}$");
    bool match = regExp.match(pass).hasMatch();
    if(!match){
        AddTipErr(TipErr::TIP_PWD_ERR,tr("不能包含非法字符"));
        return false;
    }
    DelTipErr(TipErr::TIP_PWD_ERR);

    return true;
}

bool ResetDialog::checkVerifyValid()
{
    auto verify = ui->varify_edit->text();
    if(verify.isEmpty()){
        AddTipErr(TipErr::TIP_VERIFY_ERR, tr("验证码不能为空"));
        return false;
    }
    DelTipErr(TipErr::TIP_VERIFY_ERR);
    return true;
}

void ResetDialog::slot_reset_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    if(err != ErrorCodes::SUCCESS){
        showTip(tr("网络请求错误"), false);
        return;
    }
    //解析JSON res转换为QByteArray
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    if(jsonDoc.isNull()){
        showTip(tr("json解析失败"), false);
        return;
    }
    //json解析错误
    if(!jsonDoc.isObject()){
        showTip(tr("json解析失败"), false);
        return;
    }

    _handlers[id](jsonDoc.object());
    return;
}

void ResetDialog::on_return_btn_clicked()
{
    qDebug() << "return btn clicked ";
    emit switchLogin();
}

void ResetDialog::on_varify_btn_clicked()
{
    qDebug() << "varify btn clicked ";
    auto email = ui->email_edit->text();
    auto bcheck = checkEmailValid();
    if(!bcheck){
        return;
    }
    //send http request to get verify code
    QJsonObject json_obj;
    json_obj["email"] = email;
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix+"/get_varifycode"), json_obj, ReqId::ID_RESET_PWD, Modules::RESETMOD);
}

void ResetDialog::on_sure_btn_clicked()
{
    qDebug() << "sure btn clicked ";
    bool vaild = checkUserValid();
    if(!vaild){
        return;
    }
    vaild = checkEmailValid();
        if(!vaild){
            return;
        }
    vaild = checkPassValid();
        if(!vaild){
            return;
        }
    vaild = checkVerifyValid();
        if(!vaild){
            return;
        }
    //send http request to reset user's pwd
    QJsonObject json_obj;
    json_obj["user"] = ui->user_edit->text();
    json_obj["email"] = ui->email_edit->text();
    json_obj["passwd"] = xorString(ui->pwd_edit->text());
    json_obj["verifycode"] = ui->varify_edit->text();
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/reset_pwd"), json_obj, ReqId::ID_RESET_PWD, Modules::RESETMOD);
}
