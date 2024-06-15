﻿#include "registerdialog.h"
#include "ui_registerdialog.h"
#include "global.h"
#include "httpmgr.h"
RegisterDialog::RegisterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegisterDialog), _countdown(5)
{
    ui->setupUi(this);
    ui->pass_edit->setEchoMode(QLineEdit::Password);
    ui->confirm_edit->setEchoMode(QLineEdit::Password);
    ui->err_tip->setProperty("state", "normal");
    repolish(ui->err_tip);
    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_reg_mod_finish, this, &RegisterDialog::slot_reg_mod_finish);
    initHttpHandlers();
    ui->err_tip->clear();

    connect(ui->user_edit, &QLineEdit::editingFinished, this,[this](){
        checkUserValid();
    });
    connect(ui->email_edit, &QLineEdit::editingFinished, this,[this](){
        checkEmailValid();
    });
    connect(ui->pass_edit, &QLineEdit::editingFinished, this,[this](){
        checkPasswdValid();
    });
    connect(ui->confirm_edit, &QLineEdit::editingFinished, this,[this](){
        checkConfirmValid();
    });
    connect(ui->varify_edit, &QLineEdit::editingFinished, this,[this](){
        checkVerifyValid();
    });

    ui->pass_visible->setCursor(Qt::PointingHandCursor);
    ui->confirm_visible->setCursor(Qt::PointingHandCursor);

    ui->pass_visible->SetState("unvisible", "unvisible_hover", "",
                               "visible", "visible_hover", "");
    ui->confirm_visible->SetState("unvisible", "unvisible_hover", "",
                                  "visible", "visible_hover", "");

    connect(ui->pass_visible, &ClickedLabel::clicked, this, [this](){
        auto state = ui->pass_visible->GetCurState();
        if(state == ClickLbState::Normal){
            ui->pass_edit->setEchoMode(QLineEdit::Password);
        }else{
            ui->pass_edit->setEchoMode(QLineEdit::Normal);
        }
        qDebug()<<"Label is clicked";
    });

    connect(ui->confirm_visible, &ClickedLabel::clicked, this, [this](){
        auto state = ui->confirm_visible->GetCurState();
        if(state == ClickLbState::Normal){
            ui->confirm_edit->setEchoMode(QLineEdit::Password);
        }else{
            ui->confirm_edit->setEchoMode(QLineEdit::Normal);
        }
        qDebug()<<"Label is clicked";
    });

    _countdown_timer = new QTimer(this);
    connect(_countdown_timer, &QTimer::timeout, [this](){
        if(_countdown == 0){
            _countdown_timer->stop();
            emit sigSwitchLogin();
            return;
        }
        _countdown--;
        auto str = QString("注册成功，%1 s后返回登录界面").arg(_countdown);
        ui->tip_lb->setText(str);
    });

}

RegisterDialog::~RegisterDialog()
{
    qDebug()<<"destruct RegisterDialog";
    delete ui;
}

void RegisterDialog::on_get_code_clicked()
{
    auto email = ui->email_edit->text();
    QRegularExpression regex(R"((\w+)(\. | _)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch();
    if(match){
        //发送http验证码
        QJsonObject json_obj;
        json_obj["email"] = email;
        HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix + "/get_varifycode"), json_obj, ReqId::ID_GET_VERIFY_CODE, Modules::REGISTERMOD);
    }else{
        showTip(tr("邮箱地址不正确"),false);
    }
}

void RegisterDialog::slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err)
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

void RegisterDialog::initHttpHandlers()
{
    //注册获取验证码回包的逻辑
    _handlers.insert(ReqId::ID_GET_VERIFY_CODE,[this](const QJsonObject& jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"), false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("验证码已发送，请注意查收"), true);
        qDebug() << "email is" << email;
    });
    //注册用户回包的逻辑
    _handlers.insert(ReqId::ID_REG_USER,[this](const QJsonObject& jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"), false);
            return;
        }
        auto user = jsonObj["user"].toString();
        showTip(tr("用户注册成功"), true);
        qDebug() << "user is" << user;
        qDebug() << "user uuid is" << jsonObj["uid"].toString();
        ChangeTipPage();
    });
}

void RegisterDialog::showTip(QString str, bool b_ok)
{
    if(b_ok){
        ui->err_tip->setProperty("state", "normal");
    }else{
        ui->err_tip->setProperty("state", "err");
    }
    ui->err_tip->setText(str);
    repolish(ui->err_tip);
}

void RegisterDialog::AddTipErr(TipErr te, QString tips)
{
    _tip_errs[te] = tips;
    showTip(tips,false);
}

void RegisterDialog::DelTipErr(TipErr te)
{
    _tip_errs.remove(te);
    if(_tip_errs.empty()){
        ui->err_tip->clear();
        return;
    }

    showTip(_tip_errs.first(),false);
}

bool RegisterDialog::checkUserValid()
{
    if(ui->user_edit->text() == ""){
        AddTipErr(TipErr::TIP_USER_ERR,"用户名不能为空");
        return false;
    }
    DelTipErr(TipErr::TIP_USER_ERR);
    return true;
}

bool RegisterDialog::checkEmailValid()
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

bool RegisterDialog::checkPasswdValid()
{
    auto pass = ui->pass_edit->text();
    auto confirm = ui->confirm_edit->text();
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

    if(pass != confirm){
        AddTipErr(TipErr::TIP_PWD_CONFIRM,tr("两次输入密码不一致"));
        return false;
    }else{
        DelTipErr(TipErr::TIP_PWD_CONFIRM);
    }
    return true;
}

bool RegisterDialog::checkConfirmValid()
{
    auto pass = ui->pass_edit->text();
    auto confirm = ui->confirm_edit->text();
    if(confirm.length() < 6 || confirm.length() > 15){
        AddTipErr(TipErr::TIP_CONFIRM_ERR, tr("密码长度应为6-15"));
        return false;
    }
    //正则解释
    // ^[a-zA-Z0-9!@#$%^&*]{6,15}$ 密码长度至少6，可以是字母、数字和特定的特殊字符
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*]{6,15}$");
    bool match = regExp.match(confirm).hasMatch();
    if(!match){
        AddTipErr(TipErr::TIP_CONFIRM_ERR,tr("不能包含非法字符"));
        return false;
    }
    DelTipErr(TipErr::TIP_CONFIRM_ERR);

    if(pass != confirm){
        AddTipErr(TipErr::TIP_PWD_CONFIRM,tr("两次输入密码不一致"));
        return false;
    }else{
        DelTipErr(TipErr::TIP_PWD_CONFIRM);
    }
    return true;
}

bool RegisterDialog::checkVerifyValid()
{
    auto verify = ui->varify_edit->text();
    if(verify.isEmpty()){
        AddTipErr(TipErr::TIP_VERIFY_ERR, tr("验证码不能为空"));
        return false;
    }
    DelTipErr(TipErr::TIP_VERIFY_ERR);
    return true;
}

void RegisterDialog::ChangeTipPage()
{
    _countdown_timer->stop();
    ui->stackedWidget->setCurrentWidget(ui->page_2);

    _countdown_timer->start(1000);
}

void RegisterDialog::on_sure_btn_clicked()
{     
        bool vaild = checkUserValid();
        if(!vaild){
            return;
        }

        vaild = checkEmailValid();
        if(!vaild){
            return;
        }

        vaild = checkPasswdValid();
        if(!vaild){
            return;
        }

        vaild = checkConfirmValid();
        if(!vaild){
            return;
        }

        vaild = checkVerifyValid();
        if(!vaild){
            return;
        }
        QJsonObject json_obj;
        json_obj["user"] = ui->user_edit->text();
        json_obj["email"] = ui->email_edit->text();
        json_obj["passwd"] = xorString(ui->pass_edit->text());
        json_obj["confirm"] = xorString(ui->confirm_edit->text());
        json_obj["verifycode"] = ui->varify_edit->text();
        HttpMgr::GetInstance()->PostHttpReq(gate_url_prefix + "/user_register",json_obj,ReqId::ID_REG_USER,Modules::REGISTERMOD);

}
void RegisterDialog::on_return_btn_clicked()
{
    _countdown_timer->stop();
    emit sigSwitchLogin();
}

void RegisterDialog::on_cancel_btn_clicked()
{
    _countdown_timer->stop();
    emit sigSwitchLogin();
}