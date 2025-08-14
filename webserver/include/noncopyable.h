#pragma once

/**
 * noncopyable被继承后 派生类对象可正常构造和析构 但派生类对象无法进行拷贝构造和赋值构造
 **/

 class noncopyable {
    public:
        noncopyable(const noncopyable &) = delete;  //= delete是C++11引入的特性，表示彻底禁止这个操作，这里禁用了拷贝构造函数
        noncopyable &operator=(const noncopyable &) = delete; //禁止赋值构造函数
    protected: // protected成员函数可以被派生类访问，但阻止直接实例化基类
        noncopyable() = default; //默认构造函数
        ~noncopyable() = default; //默认析构函数
 };