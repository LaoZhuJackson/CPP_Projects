// 这是现代C/C++中广泛使用的头文件保护指令，比传统的#ifndef更简洁高效。
#pragma once

#include <functional> //提供std::function和std::bind等函数包装器
#include <memory> //智能指针
#include<vector> 
# include<atomic>//提供原子操作,避免多线程下对布尔值的竞争条件（比互斥锁更轻量）
#include<mutex> //提供互斥锁

#include "noncopyable.h" //提供不可拷贝的基类
#include "Timestamp.h" //时间戳类
#include "CurrentThread.h" //当前线程相关的函数和变量
#include "TimerQueue.h" //定时器队列类
class Channel;
class Poller;

class EventLoop : noncopyable{
private:
    void handleRead(); //给eventfd返回的文件描述符wakeupFd_绑定的事件回调 当wakeup()时 即有事件发生时 调用handleRead()读wakeupFd_的8字节 同时唤醒阻塞的epoll_wait
    void doPendingFunctors(); //执行待处理的回调函数

    using ChannelList = std::vector<Channel*>; //通道列表类型

    std::atomic_bool looping_; //原子布尔值，表示事件循环是否正在运行
    std::atomic_bool quit_; //原子布尔值，表示是否退出事件循环

    const pid_t threadId_; //线程ID，确保事件循环在创建它的线程中运行
    Timestamp pollReturnTime_; //上次poll的返回时间
    std::unique_ptr<Poller> poller_; //轮询器，用于处理事件
    std::unique_ptr<TimerQueue> timerQueue_; //定时器队列，用于处理定时事件
    int wakeupFd_; //当mainLoop获取一个新用户的Channel 需通过轮询算法选择一个subLoop 通过该成员唤醒subLoop处理Channel
    std::unique_ptr<Channel> wakeupChannel_; //唤醒通道，用于处理wakeupFd_的事件

    ChannelList activeChannels_; //活动通道列表，存储当前活跃的通道

    std::atomic_bool callingPendingFunctors_; //标识当前loop是否有需要执行的回调操作
    std::vector<Functor> pendingFunctors_; //存储loop需要执行的所有回调操作
    std::mutex mutex_; //互斥锁，保护上面vector容器的线程安全操作
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    void loop(); //开始事件循环
    void quit(); //退出事件循环

    Timestamp pollReturnTime() const {return pollReturnTime_;} //获取上次poll的返回时间

    void runInLoop(Functor cb); //在事件循环中执行回调函数
    void queueInLoop(Functor cb); //将回调函数排入队列

    void updateChannel(Channel* channel); //更新通道
    void removeChannel(Channel* channel); //移除通道
    bool hasChannel(Channel* channel); //检查通道是否存在

    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); } //检查当前线程是否为事件循环所在的线程,threadId_为EventLoop创建时的线程id CurrentThread::tid()为当前线程id
    /**
     * 定时任务相关函数
     */
    void runAt(Timestamp timestamp, Functor &&cb){
    timerQueue_->addTimer(std::move(cb), timestamp, 0.0);
    }
    void runAfter(double waitime,Functor &&cb){
        Timestamp time(addTimer(Timestamp::now(),waitime));
        runAt(time,std::move(cb));
    }
    void runEvery(double interval,Functor &&cb){
        Timestamp timestamp(addTimer(Timestamp::now(), interval));
        timerQueue_->addTimer(std::move(cb), timestamp, interval);
    }
};

