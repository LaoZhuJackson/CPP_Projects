#include "Logger.h"
#include "CurrentThread.h"

namespace ThreadInfo
{
    //线程局部存储关键字，用于声明一个变量，使得 每个线程拥有该变量的独立副本
    thread_local char t_errnobuf[512]; // 每个线程独立的错误信息缓冲
    thread_local char t_timer[64];     // 每个线程独立的时间格式化缓冲区
    thread_local time_t t_lastSecond;  // 每个线程记录上次格式化的时间
}

// 将错误码（errno 值）转换为可读的错误信息字符串，并返回该字符串的指针
const char* getErrnoMsg(int savedErrno){
    return strerror_r(savedErrno, ThreadInfo::t_errnobuf,sizeof(ThreadInfo::t_errnobuf));
}

// 全局数组变量
const char* getLevelName[Logger::LogLevel::LEVEL_COUNT]{
    "TRACE ",
    "DEBUG ",
    "INFO  ",
    "WARN  ",
    "ERROR ",
    "FATAL ",
};
