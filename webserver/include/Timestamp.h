#pragma once

#include<iostream>
#include<string>
#include<sys/time.h> //提供时间相关的函数和结构体

class Timestamp{
private:
    int64_t microSecondsSinceEpoch_; // 存储自纪元以来的微秒数
public:
    Timestamp() : microSecondsSinceEpoch_(0) {} // 默认构造函数，初始化为0微秒
    //explicit：禁止隐式转换，避免int64_t意外转为Timestamp
    explicit Timestamp(int64_t microSecondsSinceEpoch) : microSecondsSinceEpoch_(microSecondsSinceEpoch) {} // 带参数的构造函数

    // 获取当前时间戳
    static Timestamp now()
    std::string toString()const; //该函数不会修改类的任何成员变量（除非变量被声明为 mutable）

    //格式, "%4d年%02d月%02d日 星期%d %02d:%02d:%02d.%06d",时分秒.微秒,showMicroseconds = false则不显示微秒
    std::string toFormattedString(bool showMicroseconds = false) const; //格式化输出时间戳

    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; } // 获取自纪元以来的微秒数
    // 获取自纪元以来的秒数
    static const int kMicroSecondsPerSecond = 1000 * 1000; // 每秒的微秒数
    time_t secondsSinceEpoch() const { return static_cast<time_t>(microSecondsSince/kMicroSecondsPerSecond); }

    static Timestamp invalid() { return Timestamp(); } // 返回一个无效的时间戳,不用创建类实例：可以直接通过类名调用，Timestamp ts = Timestamp::invalid();
};

/**
 * 定时器需要比较时间戳，因此需要重载运算符
 */
 inline bool operator<(Timestamp lhs, Timestamp rhs){
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
 }

 inline bool operator==(Timestamp lhs, Timestamp rhs){
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
 }
// 如果是重复定时任务就会对此时间戳进行增加。
 inline Timestamp addTime(Timestamp timestamp, double seconds){
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
 }