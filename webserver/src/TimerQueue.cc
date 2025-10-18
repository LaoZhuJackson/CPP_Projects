#include <EventLoop.h>
#include <Channel.h>
#include <Logger.h>
#include <Timer.h>
#include <TimerQueue.h>

#include <sys/timerfd.h>
#include <unistd.h>
#include <string.h>

int createTimerfd(){
    /**
     * CLOCK_MONOTONIC：绝对时间
     * TFD_NONBLOCK：非阻塞
     */
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
    TFD_NONBLOCK | TFD_CLOEXEC);
    if(timerfd<0){
        LOG_ERROR << "Failed in timerfd_create";
    }
    return timerfd;
}

TimerQueue::TimerQueue(EventLoop* loop)
:loop_(loop),
timerfd_(createTimerfd()),
timerfdChannel_(loop_,timerfd_),
timers_(){
    // 将 TimerQueue::handleRead 成员函数绑定为 timerfdChannel_ 的读事件回调
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead,this));
    timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue(){
    timerfdChannel_.disableAll();
    timerfdChannel_.remove();
    ::close(timerfd_);
    // 删除所有定时器
    for(const Entry& timer : timers_) delete timer.second;
}

void TimerQueue::addTimer(TimerCallback cb,Timestamp when,double interval){
    Timer* timer = new Timer(std::move(cb),when,interval);
    //等价于：loop_->runInLoop([this, timer]() { 
    //          this->addTimerInLoop(timer); 
    //       });
    loop_ ->runInLoop(std::bind(&TimerQueue::addTimerInLoop,this,timer));
}

void TimerQueue::addTimerInLoop(Timer* timer){
    // 是否取代了最早的定时触发时间
    bool earliestChanged = insert(timer);
    // 我们需要重新设置timerfd_触发时间
    if(earliestChanged) resetTimerfd(timerfd_, timer->expiration());
}

// 重置timerfd
void TimerQueue::resetTimerfd(int timerfd_, Timestamp expiration){
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memset(&newValue, '\0',sizeof(newValue));
    memset(&oldValue, '\0',sizeof(oldValue));

    // 超时时间 - 现在时间
    int64_t microSecondDif = expiration.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
    if(microSecondDif<100) microSecondDif = 100;

    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(microSecondDif/Timestamp::kMicroSecondsPerSecond);
    ts.tv_nsec = static_cast<long>((microSecondDif % Timestamp::kMicroSecondsPerSecond) * 1000);
    newValue.it_value = ts;
    if(::timerfd_settime(timerfd_,0,&newValue,&oldValue)) LOG_ERROR << "timefd_settime faield()";
}

void ReadTimerFd(int timerfd){
    // 无符号整型
    uint64_t read_byte;
    ssize_t readn = ::read(timerfd,&read_byte,sizeof(read_byte));
    if(readn != sizeof(read_byte)) LOG_ERROR << "TimerQueue::ReadTimerFd read_size < 0";
}

// 返回删除的定时器节点 （std::vector<Entry> expired）
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now){
    std::vector<Entry> expired;  // 1. 创建空向量存储过期定时器
    Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));  // 2. 创建哨兵条目
    TimerList::iterator end = timers_.lower_bound(sentry);  // 3. 查找第一个未过期的定时器:由于哨兵条目的时间戳是 now 且 Timer 指针最大，这会找到第一个时间戳 >= now 的定时器
    /* 下面一行代码等价于：
    for (auto it = timers_.begin(); it != end; ++it) {
    expired.push_back(*it);  // 将元素添加到向量末尾，自动分配内存
    }
    */
    std::copy(timers_.begin(), end, back_inserter(expired));  // 4. 复制所有过期定时器
    timers_.erase(timers_.begin(), end);  // 5. 从队列中移除已过期的定时器

    return expired;
}

void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now){
    Timestamp nextExpire;
    for(const Entry& it : expired){
        // 重复任务则继续执行
        if(it.second->repeat()){
            auto timer = it.second;
            timer->restart(Timestamp::now());
            insert(timer);
        }else{
            delete it.second;
        }
        // 如果重新插入了定时器，需要继续重置timerfd
        if(!timers_.empty()) resetTimerfd(timerfd_,(timers_.begin()->second)->expiration());
    }
}

void TimerQueue::handleRead(){
    Timestamp now = Timestamp::now();
    ReadTimerFd(timerfd_);

    std::vector<Entry> expired = getExpired(now);
    // 遍历已经到点的定时器，执行对应的回调函数
    callingExpiredTimers_ = true;
    for(const Entry& it : expired){
        it.second->run();
    }
    callingExpiredTimers_ = false;
    
    // 重新设置这些定时器
    reset(expired, now);
}

bool TimerQueue::insert(Timer* timer){
    bool earliestChanged = false;
    Timestamp when = timer->expiration();
    TimerList::iterator it = timers_.begin();
    // 说明最早的定时器已经被替换了
    if(it == timers_.end() || when < it->first) earliestChanged = true;
    
    // 定时器管理红黑树插入此新定时器
    timers_.insert(Entry(when,timer));
    return earliestChanged;
}