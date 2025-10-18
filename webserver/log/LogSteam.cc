#include "LogStream.h"
#include <algorithm>

static const char digits[] = "9876543210123456789";//-123 % 10 = -3,所以负数区域也有对应的1-9

template <typename T>
void LogStream::formatInteger(T num){
    if(buffer_.avail() >= kMaxNumberSize){
        char* start = buffer_.current();
        char* cur = start;
        static const char* zero = digits + 9;//digits + 9正好指向中间的'0'字符
        bool negative = (num<0);//判断是否为负数
        do{//do-while：至少执行一次循环体
            int remainder = static_cast<int>(num % 10);
            (*cur++) = zero[remainder];//赋值后cur再++，zero[remainder] 是语法糖，编译器会转换为 *(zero + remainder)
            num /= 10;
        }while(num != 0);
        if(negative) *cur++ = '-';
        *cur = '\0';
        std::reverse(start,cur);//上面组成的字符串是和原本刚好反过来的
        int length = static_cast<int>(cur-start);
        buffer_.add(length);
    }
}
// 重载输出流运算符<<，用于将布尔值写入缓冲区
LogStream& LogStream::operator<<(bool express){// 返回引用，支持链式调用
    buffer_.append(express ? "true" : "false", express ? 4 : 5);
    return *this;
}
// 短整型
LogStream& LogStream::operator<<(short number){
    formatInteger(number);
    return *this;
}
// 整型
LogStream& LogStream::operator<<(int number){
    formatInteger(number);
    return *this;
}
// 无符号整型
LogStream& LogStream::operator<<(unsigned int number){
    formatInteger(number);
    return *this;
}
// 长整型
LogStream& LogStream::operator<<(long number){
    formatInteger(number);
    return *this;
}
// 无符号长整型
LogStream& LogStream::operator<<(unsigned long number){
    formatInteger(number);
    return *this;
}
// 长长整型
LogStream& LogStream::operator<<(long long number){
    formatInteger(number);
    return *this;
}
// 无符号长长整型
LogStream& LogStream::operator<<(unsigned long long number){
    formatInteger(number);
    return *this;
}
// 浮点型
LogStream& LogStream::operator<<(float number){
    *this<<static_cast<double>(number);//等价于：this->operator<<(static_cast<double>(number))
    return *this;
}
// 双精度浮点型
LogStream& LogStream::operator<<(double number){
    char buffer[32];
    snprintf(buffer,sizeof(buffer),"%.12g",number);//%g：智能选择%f(定点)或%e(科学计数法)格式
    buffer_.append(buffer,strlen(buffer));
    return *this;
}
// 字符型
LogStream& LogStream::operator<<(char str){
    buffer_.append(&str,1);
    return *this;
}
// c风格字符串
LogStream& LogStream::operator<<(const char* str){
    buffer_.append(str,strlen(str));//strlen遇到第一个\0就结束
    return *this;
}
// 无符号字符指针
LogStream& LogStream::operator<<(const unsigned char* str){
    /*
    unsigned char* 和 char* 是不同的类型
    但它们有相同的内存表示（都是字节）
    static_cast 不允许这种不相关指针类型的转换
    reinterpret_cast 告诉编译器："相信我，我知道我在做什么"
    */
    buffer_.append(reinterpret_cast<const char*>(str),strlen(reinterpret_cast<const char*>(str)));
    return *this;
}
// std:;string
LogStream& LogStream::operator<<(const std::string& str){
    buffer_.append(str.data(),str.size());
    return *this;
}
// 自定义的GeneralTemplate类型输出
LogStream& LogStream::operator<<(const GeneralTemplate& g)
{
    buffer_.append(g.data_, g.len_);
    return *this;
}