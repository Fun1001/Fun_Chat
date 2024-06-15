#ifndef CLICKEDLABEL_H
#define CLICKEDLABEL_H
#include <QLabel>
#include "global.h"

class ClickedLabel : public QLabel
{
    Q_OBJECT
public:
    ClickedLabel(QWidget *parent=nullptr);
    virtual void mousePressEvent(QMouseEvent *ev) override;
    virtual void enterEvent(QEvent *event) override;
    virtual void leaveEvent(QEvent *event) override;
    virtual void mouseReleaseEvent(QMouseEvent *ev) override;
    //一个Label有六种状态，普通状态，普通的悬浮状态，普通的点击状态，选中状态，选中的悬浮状态，选中的点击状态。
    void SetState(QString normal="",QString hover="",QString press="",
                  QString select="",QString select_hover="",QString select_press="");
   ClickLbState GetCurState();
private:
    QString _normal;
    QString _normal_hover;
    QString _normal_press;

    QString _selected;
    QString _selected_hover;
    QString _selected_press;

    ClickLbState _curstate;


signals:
    void clicked(void);
};

#endif // CLICKEDLABEL_H
