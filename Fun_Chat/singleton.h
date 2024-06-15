#ifndef SINGLETON_H
#define SINGLETON_H
#include "global.h"
template <typename T>
class Singleton{
protected:
    Singleton() = default;
    Singleton(const Singleton<T>&) = delete;
    Singleton& operator = (const Singleton<T>& st) =delete;
    static std::shared_ptr<T> _instance;//必须要初始化，非模板类在.cpp中初始化
public:
    static std::shared_ptr<T> GetInstance(){
        static std::once_flag s_flag;
        std::call_once(s_flag, [&](){
            _instance = std::shared_ptr<T>(new T);//用new调用T而不用make_share:因为在子类继承时构造函数为私有，make_share无法访问非公有构造函数
        });
        return _instance;
    }

    void PrintAddress(){
        std::cout << _instance.get() << std::endl;
    }
    ~Singleton(){
        std::cout << "this is singleton destruct" << std::endl;
    }
};

template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;

#endif // SINGLETON_H
