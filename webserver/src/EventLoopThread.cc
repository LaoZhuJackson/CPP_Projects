#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb, const std::string& name)
    : loop_(nullptr)
    , exiting_(false)
    , thread_(std::bind(&EventLoopThread::threadFunc,this),name)
    , mutex_()
    , cond_()
    , callback_(cb){}

EventLoopThread::~EventLoopThread(){
    exiting_ = true;
    if(loop_ != nullptr){
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop(){
    thread_.start();// 启用底层线程Thread类对象thread_中通过start()创建的线程
    EventLoop* loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock,[this](){return loop_ != nullptr;});
        loop = loop_;
    }
    return loop;
}
// 下面这个方法 是在单独的新线程里运行的
void EventLoopThread::threadFunc(){
    EventLoop loop;// 每个线程有自己独立的EventLoop实例

    if(callback_) callback_(&loop);

    {
        std::unique_lock<std::mutex> lock(mutex_);// 确保主线程看到的是完全初始化后的loop指针
        loop_ = &loop;        // 将loop指针赋给成员变量
        cond_.notify_one();   // 通知等待的主线程
    }
    loop.loop();// 执行EventLoop的loop() 开启了底层的Poller的poll()
    std::unique_lock<std::mutex> lock(mutex_);// 确保线程安全地清理共享资源
    loop_ = nullptr;
}