﻿#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include "global.h"
#include "httpmgr.h"
namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

private:
    Ui::LoginDialog *ui;
    void initHttpHandlers();
    void initHead();
    bool checkEmailValid();
    bool checkPassValid();
    void showTip(QString str, bool b_ok);
    void AddTipErr(TipErr te, QString tips);
    void DelTipErr(TipErr te);
    bool enableBtn(bool enabled);

    int _uid;
    QString _token;
    QMap<TipErr, QString> _tip_errs;
    QMap<ReqId,std::function<void(const QJsonObject&)>> _handlers;
signals:
    void switchRegister();
    void switchReset();
    void sig_connect_tcp(ServerInfo);

private slots:
    void on_login_btn_clicked();
    void slot_forget_pwd();
    void slot_login_mod_finish(ReqId id, QString res, ErrorCodes err);
    void slot_tcp_con_finish(bool bsuccess);
    void slot_login_failed(int err);
};

#endif // LOGINDIALOG_H
