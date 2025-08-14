#pragma once

#include <pthread.h> // 本地存储
#include <stdio.h>
#include <string.h>
#include <string>
#include <errno.h>
#include "LogStream.h"
#include<functional>
#include "Timestamp.h"

#define OPEN_LOGGING // 定义了一个宏，通过宏定义可以在不修改代码的情况下，控制功能开关

// SourceFile的作用是提取文件名
class SourceFile{
public:
    const char* data_;
    int size_;

    explicit SourceFile(const char* filename) : data_(filename){
        /**
         * 找出data中出现/最后一次的位置，从而获取具体的文件名
         * 2022/10/26/test.log
         */
        const char* slash = strrchr(filename, '/');
        if(slash){
            data_ = slash + 1;
        }
        // strlen(data_)计算的是从data_指向的位置开始，到字符串结束符'\0'的字符数
        size_ = static_cast<int>(strlen(data_)); //返回文件名的长度
    }
};

class Logger{
public:
    enum LogLevel{
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        LEVEL_COUNT,
    };
    Logger(const char *filename,int line, LogLevel level);
    ~Logger();
    // 流是会改变的
    LogStream& stream() { return impl_.stream_; }

    // 输出函数和刷新缓冲区函数
    using OutputFunc = std::function<void(const char* msg,int len)>;
    using FlushFunc = std::function<void()>;
    static void setOutput(OutputFunc);
    static void setFlush(FlushFunc);
private:
    class Impl{
    public:
        using LogLevel = Logger::LogLevel;
        Impl(LogLevel level,int savedErrno,const char* filename,int line);
        void formatTime();
        void finish(); // 添加一条log消息的后缀

        Timestamp time_;
        LogStream stream_;
        LogLevel level_;
        int line_;
        SourceFile basename_;
    };
private:
    Impl impl_;
};

// 获取errno信息
const char* getErrnoMsg(int savedErrno);
/**
 * 当日志等级小于对应等级才会输出
 * 比如设置等级为FATAL，则logLevel等级大于DEBUG和INFO，DEBUG和INFO等级的日志就不会输出
 */
#ifdef OPEN_LOGGING
#define LOG_DEBUG Logger(__FILE__, __LINE__, Logger::DEBUG).stream()
#define LOG_INFO Logger(__FILE__, __LINE__, Logger::INFO).stream()
#define LOG_WARN Logger(__FILE__, __LINE__, Logger::WARN).stream()
#define LOG_ERROR Logger(__FILE__, __LINE__, Logger::ERROR).stream()
#define LOG_FATAL Logger(__FILE__, __LINE__, Logger::FATAL).stream()
#else
#define LOG(level) LogStream()
#endif

