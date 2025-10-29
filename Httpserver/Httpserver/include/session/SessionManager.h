#pragma once

#include "SessionStorage.h"
#include "../http/HttpRequest.h"
#include "../http/HttpResponse.h"
#include <memory>
#include <random>

namespace http::session{
SessionManager{
public:
    explicit SessionManager(std::unique_ptr<SessionStorage> storage);

    // 从请求中创建或获取对话
    std::shared_ptr<Session> getSession(const HttpRequest& req, HttpResponse* resp);
    //销毁对话
    void destroySession(const std::string& sessionId);
    //清理过期对话
    void cleanExpiredSessions();
    //更新对话
    void updateSession(std::shared_ptr<Session> session){
        storage_->save(session);
    }
private:
    std::string generateSessionId();
    std::string getSessionIdFromCookie(const HttpRequest* req);
    void setSessionCookie(const std::string& sessionId,,HttpResponse* resp);

private:
    std::unique_ptr<SessionStorage> storage_;
    std::mt19937 rng_; //用于生成随机会话id，比 rand() 函数更好的统计特性
};
}