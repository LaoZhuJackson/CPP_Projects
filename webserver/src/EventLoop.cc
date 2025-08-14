#include<sys/eventfd.h> //提供eventfd函数
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

#include <EventLoop.h>
#include <Logger.h>
#include <Channel.h>
#include <Poller.h>
// 防止一个线程中创建多个EventLoop实例
thread_local EventLoop* t_loopInThisThread = nullptr; //线程局部存储，确保每个线程有自己的EventLoop实例

const int kPollTimeMs = 10000 //10秒

/* 创建线程之后主线程和子线程谁先运行是不确定的。
 * 通过一个eventfd在线程之间传递数据的好处是多个线程无需上锁就可以实现同步。
 * eventfd支持的最低内核版本为Linux 2.6.27,在2.6.26及之前的版本也可以使用eventfd，但是flags必须设置为0。
 * 函数原型：
 *     #include <sys/eventfd.h>
 *     int eventfd(unsigned int initval, int flags);
 * 参数说明：
 *      initval,初始化计数器的值。
 *      flags, EFD_NONBLOCK,设置socket为非阻塞。
 *             EFD_CLOEXEC，执行fork的时候，在父进程中的描述符会自动关闭，子进程中的描述符保留。
 * 场景：
 *     eventfd可以用于同一个进程之中的线程之间的通信。
 *     eventfd还可以用于同亲缘关系的进程之间的通信。
 *     eventfd用于不同亲缘关系的进程之间通信的话需要把eventfd放在几个进程共享的共享内存中（没有测试过）。
 */

 int createEventfd(){
   //::eventfd明确表示调用全局命名空间中的eventfd函数，而不是任何可能存在的类成员或局部作用域中的同名函数。
   int evtfd = ::eventfd(0, EFD_)
 }