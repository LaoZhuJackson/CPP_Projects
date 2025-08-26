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

/**
 * 默认的日志输出函数
 * 将日志内容写入标准输出流(stdout)
 * @param data 要输出的日志数据
 * @param len 日志数据的长度W
 */
static void defaultOutput(const char *data,int len){
    fwrite(data, len, sizeof(char),stdout);
}

/**
 * 默认的刷新函数
 * 刷新标准输出流的缓冲区,确保日志及时输出
 * 在发生错误或需要立即看到日志时会被调用
 */
static void defaultFlush(){
    fflush(stdout);
}
Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;

// 实现Impl构造函数
Logger::Impl::Impl(Logger::LogLevel level,int savedErrno,const char* filename,int line)
    : time_(Timestamp::now()), // 显式初始化 time_
      stream_(),              // 显式调用 LogStream 的默认构造函数,与其他成员变量的初始化风格保持一致（所有成员都在初始化列表中显式处理）
      level_(level),          // 初始化 level_
      line_(line),            // 初始化 line_
      basename_(filename)     // 初始化 basename_
    {
    // 显示调用格式化时间
    formatTime();
    // 写入日志等级:通过 GeneralTemplate 类将日志级别名称（getLevelName[level]）及其固定长度（6）封装为一个轻量级对象，然后通过重载的 << 运算符将其写入日志流（stream_）
    stream_<<GeneralTemplate(getLevelName[level],6);
    // 写入errno
    if(savedErrno != 0){
        stream_<< getErrnoMsg(savedErrno) << "(errno=" << savedErrno << ")";
    }
}
// formatTime实现：根据时区格式化当前时间字符串, 也是一条log消息的开头
void Logger::Impl::formatTime(){
    Timestamp now = Timestamp::now();
    // 计算秒数
    time_t seconds = static_cast<time_t>(now.microSecondsSinceEpoch() / Timestamp::kMicroSecondsPerSecond);
    int microseconds = static_cast<int>(now.microSecondsSinceEpoch() % Timestamp::kMicroSecondsPerSecond);
    // 计算剩余微秒数
    struct tm* tm_timer = localtime(&seconds);
    // 写入当前线程存储时间的buf中
    snprintf(ThreadInfo::t_timer,sizeof(ThreadInfo::t_timer),"%4d/%02d/%02d %02d:%02d:%02d",
        tm_timer->tm_year + 1900,
        tm_timer->tm_mon + 1,
        tm_timer->tm_mday,
        tm_timer->tm_hour,
        tm_timer->tm_min,
        tm_timer->tm_sec
    );
    // 更新最后一次时间调用
    ThreadInfo::t_lastSecond = seconds;
    // muduo使用Fmt格式化整数，这里我们直接写入buf
    char buf[32] = {0};
    snprintf(buf, sizeof(buf), "%06d ", microseconds);

    stream_ << GeneralTemplate(ThreadInfo::t_timer, 17) << GeneralTemplate(buf,7);
}
void Logger::Impl::finish(){
    stream_ << " - " << GeneralTemplate(basename_.data_, basename_.size_) << ':' << line_ << '\n';
}

Logger::Logger(const char *filename, int line, LogLevel level) : impl_(level, 0, filename, line)
{
}
Logger::~Logger(){
    impl_.finish();
    const LogStream::Buffer &buffer = stream().buffer();
    // 输出(默认项终端输出)
    g_output(buffer.data(),buffer.length());
    // 处理FATAL情况
    if(impl_.level_ == FATAL){
        g_flush();
        abort();
    }
}
// 使用自定义输出流、刷新方法
void Logger::setOutput(OutputFunc out){
    g_output = out;
}

void Logger::setFlush(FlushFunc flush){
    g_flush = flush;
}