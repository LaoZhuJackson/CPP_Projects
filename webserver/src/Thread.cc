#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func,const std::string &name):started_(false),joined_(false),tid_(0),func_(std::move(func)),name_(name){
    setDefaultName();
}

Thread::~Thread(){
    if(started_ && !joined_)  // 如果线程已启动但未被等待
        thread_->detach();    // thread类提供了设置分离线程的方法 线程运行后自动销毁（非阻塞）
}

void Thread::start(){ // 一个Thread对象 记录的就是一个新线程的详细信息
    started_ = true;
    sem_t sem;
    sem_init(&sem,false,0);// false指的是 不设置进程间共享,信号量设置为0
    // 开启新线程,使用lambda按引用捕获所有外部变量
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        tid_ = CurrentThread::tid();// 获取线程的tid值
        sem_post(&sem);//增加信号量的值（从0变成1）表示准备好了
        func_();// 开启一个新线程 专门执行该线程函数
    }));
    // 这里必须等待获取上面新创建的线程的tid值
    sem_wait(&sem);
}
// C++ std::thread 中join()和detach()的区别：https://blog.nowcoder.net/n/8fcd9bb6e2e94d9596cf0a45c8e5858a
void Thread::join(){
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName(){
    int num = ++numCreated_;
    if(name_.empty()){
        char buf[32] = {0};
        snprintf(buf,sizeof buf,"Thread%d",num);//snprintf 是 C 标准库中的一个函数，用于将格式化的数据安全地写入一个字符数组（缓冲区）。它是更安全的 sprintf 的替代品
        name_ = buf;
    }
}
