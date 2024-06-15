#ifndef RESETDIALOG_H
#define RESETDIALOG_H

#include <QDialog>
#include "global.h"
namespace Ui {
class ResetDialog;
}
class ResetDialog :public QDialog
{
    Q_OBJECT
public:
    explicit ResetDialog(QWidget *parent = nullptr);
    ~ResetDialog();
private:
    Ui::ResetDialog *ui;
    void initHttpHandlers();
    void showTip(QString str, bool b_ok);
    void AddTipErr(TipErr te, QString tips);
    void DelTipErr(TipErr te);
    bool checkUserValid();
    bool checkEmailValid();
    bool checkPassValid();
    bool checkVerifyValid();

    QMap<TipErr, QString> _tip_errs;
    QMap<ReqId,std::function<void(const QJsonObject&)>> _handlers;

signals:
    void switchLogin();
private slots:
    void slot_reset_mod_finish(ReqId id, QString res, ErrorCodes err);
    void on_return_btn_clicked();
    void on_varify_btn_clicked();
    void on_sure_btn_clicked();
};

#endif // RESETDIALOG_H
