#pragma once

#include<Timestamp.h> //时间戳类
#include<Channel.h> //通道类

#include<vector> //提供动态数组容器
#include<set> //提供集合容器

class EventLoop; //前向声明EventLoop类
class Timer;

class TimerQueue {
public:
private:
    // 使用别名,避免类型冗长
    using Entry = std::pair<Timestamp, Timer*>;
    using TimerList = std::set<Entry>;
    // 在本loop中添加定时器
    void addTimerInLoop(Timer* timer);
    // 定时器读事件触发函数
    void handleRead();
    // 重新设置timerfd_
    void resetTimerfd(int timerfd_, Timestamp expiration);
    // 移除所有已到期的定时器
    // 1.获取到期的定时器
    // 2.重置这些定时器（销毁或者重复定时任务）
    std::vector<Entry> getExpired(Timestamp now);
    void reset(const std::vector<Entry>& expired,Timestamp now);
    // 插入定时器的内部方法
    bool insert(Timer* timer);

    EventLoop* loop_;           // 所属的EventLoop
    const int timerfd_;         // timerfd是Linux提供的定时器接口
    Channel timerfdChannel_;    // 封装timerfd_文件描述符
    // Timer list sorted by expiration
    TimerList timers_;          // 定时器队列（set内部实现是红黑树）

    bool callingExpiredTimers_; // 标明正在获取超时定时器
};