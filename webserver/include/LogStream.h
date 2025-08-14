#pragma once
#include <string.h>
#include <string>
#include "noncopyable.h"
#include "FixedBuffer.h"

class GeneralTemplate : noncopyable{
public:
    GeneralTemplate() : data_(nullptr), len_(0){}

    explicit GeneralTemplate(const char* data, int len) : data_(data),len_(len){}

    const char* data_;
    int len_;
};

class LogStream : noncopyable{
public:
    // 定义一个Buffer类型，使用固定大小的缓冲区
    using Buffer = FixedBuffer<kSmallBufferSize>;

    // 将指定长度的字符数据追加到缓冲区
    void append(const char* buffer, int len){
        buffer_.append(buffer,len);
    }
    // 返回当前缓冲区的常量引用:返回只读的引用，避免拷贝，适合大型对象
    const Buffer& buffer() const{
        return buffer_; // 返回的是成员变量 buffer_，其生命周期与类实例绑定
    }
    // 重置缓冲区，将当前指针重置到缓冲区的起始位置
    void reset_buffer(){
        buffer_.reset();
    }
    // 重载输出流运算符<<，自定义各种类型的输出行为
    LogStream &operator << (bool express);
    // 重载输出流运算符<<，用于将短整型写入缓冲区
    LogStream &operator<<(short number);
    // 重载输出流运算符<<，用于将无符号短整型写入缓冲区
    LogStream &operator<<(unsigned short);
    // 重载输出流运算符<<，用于将整型写入缓冲区
    LogStream &operator<<(int);
    // 重载输出流运算符<<，用于将无符号整型写入缓冲区
    LogStream &operator<<(unsigned int);
    // 重载输出流运算符<<，用于将长整型写入缓冲区
    LogStream &operator<<(long);
    // 重载输出流运算符<<，用于将无符号长整型写入缓冲区
    LogStream &operator<<(unsigned long);
    // 重载输出流运算符<<，用于将长长整型写入缓冲区
    LogStream &operator<<(long long);
    // 重载输出流运算符<<，用于将无符号长长整型写入缓冲区
    LogStream &operator<<(unsigned long long);

    // 重载输出流运算符<<，用于将浮点数写入缓冲区
    LogStream &operator<<(float number);
    // 重载输出流运算符<<，用于将双精度浮点数写入缓冲区
    LogStream &operator<<(double);

    // 重载输出流运算符<<，用于将字符写入缓冲区
    LogStream &operator<<(char str);
    // 重载输出流运算符<<，用于将C风格字符串写入缓冲区
    LogStream &operator<<(const char *);
    // 重载输出流运算符<<，用于将无符号字符指针写入缓冲区
    LogStream &operator<<(const unsigned char *);
    // 重载输出流运算符<<，用于将std::string对象写入缓冲区
    LogStream &operator<<(const std::string &);
    // (const char*, int)的重载
    LogStream& operator<<(const GeneralTemplate& g);
private:
    // 静态变量：所有类实例共享，编译器常量：编译器会直接替换 kMaxNumberSize 为 32，无运行时开销，int：比define好用，有类型检查
    static constexpr int kMaxNumberSize = 32;
    template <typename T>
    void formatInteger(T num);

    // 内部缓冲区对象
    Buffer buffer_;
};