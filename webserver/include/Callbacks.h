#pragma once
#include <memory>
#include <functional>

class Buffer;
class TcpConnection;
class Timestamp;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&,size_t)>;//当发送缓冲区中的数据量达到某个预设的高水位标记时被调用

using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*,Timestamp)>;