#pragma once

#include <iostream>

#include <muduo/net/TcpServer.h>
#include "HttpRequest.h"

namespace http{
class HttpContext{
public:
    enum HttpRequestParseState{
        kExpectRequestLine, // 解析请求行
        kExpectHeaders, // 解析请求头
        kExpectBody, // 解析请求体
        kGotAll, // 解析完成
    };
    HttpContext() : state_(kEExpectRequestLine){}

    bool parseRequest(muduo::net::buffer* buf,muduo::Timestamp receiveTime);
    bool gotAll() const{return state_ == kGotAll;}

    void reset(){
        state_ = kExpectRequestLine;
        HttpRequest dummyData;
        request_.swap(dummyData);
    }
    const HttpRequest& request() const{return request_;}//只读访问，用于获取请求信息但不修改
    HttpRequest& request(){return request_;}//需要在某些情况下直接修改HttpRequest对象
private:
    bool processRequestLine(const char* begin,const char* end);
private:
    HttpRequestParseState state_;
    HttpRequest request_;
};
}