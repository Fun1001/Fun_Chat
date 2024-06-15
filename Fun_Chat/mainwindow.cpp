#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    _reg_dlg = new RegisterDialog(this);
    setCentralWidget(_login_dlg);//将_login_dlg设置到mainwindow的核心组件里面


//    _login_dlg = new LoginDialog(this);
//    setCentralWidget(_login_dlg);
//   _reg_dlg = new RegisterDialog(this);
//   _login_dlg->setWindowFlags(Qt::CustomizeWindowHint |Qt::FramelessWindowHint);
//   _reg_dlg ->setWindowFlags(Qt::CustomizeWindowHint |Qt::FramelessWindowHint);
    //创建和注册消息连接,_login_dlg会被析构
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);

    //连接登录界面忘记密码信号
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);

}
MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::SlotSwitchReg()
{
    _reg_dlg = new RegisterDialog(this);

    //将对话框嵌入到mainwindow中
    _reg_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);

    connect(_reg_dlg, &RegisterDialog::sigSwitchLogin, this, &MainWindow::SlotSwitchLogin);

    setCentralWidget(_reg_dlg);
    _login_dlg->hide();
    _reg_dlg->show();
}

void MainWindow::SlotSwitchLogin()
{
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);//将_login_dlg设置到mainwindow的核心组件里面
    _reg_dlg->hide();
    _login_dlg->show();
    //连接注册
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    //连接登录界面忘记密码信号
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
}

void MainWindow::SlotSwitchReset()
{
    _reset_dlg = new ResetDialog(this);
    _reset_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(_reset_dlg);//将_reset_dlg设置到mainwindow的核心组件里面
    _login_dlg->hide();
    _reset_dlg->show();
    //连接忘记密码
    connect(_reset_dlg, &ResetDialog::switchLogin, this, &MainWindow::SlotSwitchLogin2);
}

void MainWindow::SlotSwitchLogin2()
{
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint | Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);//将_login_dlg设置到mainwindow的核心组件里面
    _reset_dlg->hide();
    _login_dlg->show();
    //连接注册
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
}
