#pragma once

#include<unistd.h> //提供基本的C标准库函数
#include<sys/syscall.h> //提供系统调用接口

namespace CurrentThread 
{
    extern thread_local int t_cachedTid; //线程局部存储，缓存线程ID
    void cacheTid(); //缓存线程ID的函数

    inline int tid(){
        if(__builtin_expect(t_cachedTid == 0, 0)) //检查缓存的线程ID是否为0
        {
            cacheTid(); //如果是0，调用cacheTid函数获取当前线程ID
        }
        return t_cachedTid; //返回缓存的线程ID
    }
}