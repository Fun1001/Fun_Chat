#include "global.h"

extern QString gate_url_prefix="";


std::function<void(QWidget*)> repolish = [](QWidget* w){
    w->style()->unpolish(w);
    w->style()->polish(w);
};

std::function<QString(QString)> xorString = [](QString input){
    QString result = input;
    int len = input.length();
    len = len % 255;
    for(int i = 0; i<len; ++i){
        //对每一个字符进行异或操作
        result[i] = QChar(static_cast<ushort>(input[i].unicode() ^ static_cast<ushort>(len)));
    }
    return result;
};
