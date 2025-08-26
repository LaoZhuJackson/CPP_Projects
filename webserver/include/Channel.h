#pragma once
#include <functional> // 提供std::function和std::bind等函数包装器
#include <memory> // 智能指针

#include "noncopyable.h" // 提供不可拷贝的基类
#include "Timestamp.h" // 时间戳类

class EventLoop; //这一行代码是一个 前向声明

/**
 * 理清楚 EventLoop、Channel、Poller之间的关系  Reactor模型上对应多路事件分发器
 * Channel理解为通道 封装了sockfd和其感兴趣的event 如EPOLLIN、EPOLLOUT事件 还绑定了poller返回的具体事件
 **/
class Channel:noncopyable{

public:
    using EventCallback = std::function<void()>; // 取别名：定义事件回调类型为一个函数包装器，可以接受无参数并返回void的函数
    using ReadEventCallback = std::function<void(Timestamp)>; // 取别名：定义读事件回调类型为一个函数包装器，可以接受一个时间戳参数并返回void的函数

    Channel(EventLoop* loop, int fd);
    ~Channel();

    void handleEvent(Timestamp receiveTime); // 处理事件:fd得到Poller通知以后 处理事件 handleEvent在EventLoop::loop()中调用

    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); } // 设置读事件回调
    void setWriteCallback(EventCallback cb){writeCallback_ = std::move(cb);} // 设置写事件回调
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); } // 设置关闭事件回调
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); } // 设置错误事件回调

    void tie(const std::shared_ptr<void>&); // 绑定一个shared_ptr到Channel:防止当channel被手动remove掉 channel还在执行回调操作
    int fd() const { return fd_; } // 获取文件描述符
    int events() const { return events_; } // 获取事件类型
    void set_revents(int revt) { revents_ = revt; } // 设置返回事件

    // 位运算设置fd相应的事件状态 相当于epoll_ctl add delete
    void enableReading() {events_ |= kReadEvent; update();}
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }

    // 返回fd当前的事件状态
    bool isNoneEvent() const {return events_ == kNoneEvent;}
    // 位与运算，结果非0则为true
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    int index(){return index_;}
    void set_index(int idx){index_ = idx;}
    // one loop per thread
    EventLoop* ownerLoop() {return loop_;}
    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime); // 处理事件的函数，带有时间戳参数

    static const int kNoneEvent; // 无事件
    static const int kReadEvent; // 读事件
    static const int kWriteEvent; // 写事件

    EventLoop* loop_; // 事件循环指针
    const int fd_; // 文件描述符
    int events_; // 注册fd感兴趣的事件
    int revents_; // Poller返回的具体发生事件
    int index_;

    std::weak_ptr<void> tie_; // 用于防止Channel被手动remove掉时还在执行回调操作
    bool tied_; // 是否绑定了shared_ptr

    // 因为channel通道里可获知fd最终发生的具体的事件events，所以它负责调用具体事件的回调操作
    ReadEventCallback readCallback_; // 读事件回调
    EventCallback writeCallback_; // 写事件回调
    EventCallback closeCallback_; // 关闭事件回调
    EventCallback errorCallback_; // 错误事件回调
};