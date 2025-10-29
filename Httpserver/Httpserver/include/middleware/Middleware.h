#pragma once

#include "../http/HttpRequest.h"
#include "../http/HttpResponse.h"

namespace http::middleware{
class Middleware{
public:
    virtual ~Middleware() = default;
    //请求前处理
    virtual void before(HttpRequest& request) = 0;//纯虚函数，必须被重写
    // 响应后处理
    virtual void after(HttpResponse& response) = 0;
    // 设置下一个中间件
    void setNext(std::shared_ptr<Middleware> next) nextMiddleware_ = next;
protected:
    std::shared_ptr<Middleware> nextMiddleware_;
};
}