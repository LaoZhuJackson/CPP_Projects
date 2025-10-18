#include <string>

#include "TcpServer.h"
#include "Logger.h"
#include <sys/stat.h>
#include <sstream>
#include "AsyncLogging.h"
#include "LFU.h"
#include "memoryPool.h"
//日志滚动大小为1MB（1*1024*1024）
static const off_t kRollSize = 1*1024*1024;
class EchoServer{
public:
    EchoServer(EventLoop* loop,const InetAddress& addr,const std::string& name):server_(loop,addr,name),loop_(loop){
        //注册回调函数
        server_.setConnectionCallback(std::bind(&EchoServer::onConnection,this,std::placeholders::_1));
        server_.setMessageCallback(std::bind(&EchoServer::onMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3));
        //设置合适的subloop线程数量
        server_.setThreadNum(3);
    }
    void start() {server_.start();}
private:
    TcpServer server_;
    EventLoop* loop_;

    //连接建立或断开的回调函数
    void onConnection(const TcpConnectionPtr& conn){
        if(conn->connected()) LOG_INFO<<"Connection UP :"<<conn->peerAddress().toIpPort().c_str();
        else LOG_INFO<<"Connection DOWN :"<<conn->peerAddress().toIpPort().c_str();
    }
    //可读写事件回调
    void onMessage(const TcpConnectionPtr& conn,Buffer* buf,Timestamp time){
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);
    }
};

AsyncLogging* g_asyncLog = NULL;
AsyncLogging* getAsyncLog(){return g_asyncLog;}

void asyncLog(const char* msg,int len){
    AsyncLogging* logging = getAsyncLog();
    if(logging) logging->append(msg,len);
}

int main(int argc, char* argv[]){
    //启动日志，双缓冲异步写入磁盘
    //创建一个文件夹
    const std::string LogDir="logs";
    mkdir(LogDir.c_str(),0755);
    //使用std::stringstream构建日志文件夹,ostringstream 将内容输出到一个内部的字符串对象中
    std::ostringstream LogFilePath;
    LogFilePath<<LogDir<<"/"<<::basename(argv[0]); //完整的日志文件路径
    AsyncLogging log(LogFilePath.str(),kRollSize);
    g_asyncLog = &log;
    Logger::setOutput(asyncLog);// 为Logger设置输出回调,重新配接输出位置
    log.start(); //开启日志后端系统
    //第二步启动内存池和LFU缓存
    // 初始化内存池
    memoryPool::HashBucket::initMemoryPool();

    //初始化缓存
    const int CAPACITY = 5;
    KamaCache::KLfuCache<int,std::string> lfu(CAPACITY);
    //第三步启动底层网络模块
    EventLoop loop;
    InetAddress addr(8080);
    EchoServer server(&loop, addr,"EchoServer");
    server.start();
    // 主loop开始事件循环，epoll_wait阻塞 等待就绪事件（主loop只注册了监听套接字的fd，所以只会处理新连接事件）
    std::cout << "================================================Start Web Server================================================" << std::endl;
    loop.loop();
    std::cout << "================================================Stop Web Server=================================================" << std::endl;
    //结束日志打印
    log.stop();
}