#include <strings.h>
#include <string.h>

#include "InetAddress.h"

InetAddress::InetAddress(uint16_t port, std::string ip){
    ::memset(&addr_,0,sizeof(addr_));
    addr_.sin_family = AF_INET;//声明使用IPv4地址格式
    addr_.sin_port = ::htons(port); // 本地字节序转为网络字节序
    addr_.sin_addr.s_addr = ::inet_addr(ip.c_str());
}

std::string InetAddress::toIp() const{
    char buf[64] = {0};
    // sizeof 作为运算符,当传入的是类型使必须有括号，传入的是变量时可以没有括号
    ::inet_ntop(AF_INET,&addr_.sin_addr,buf, sizeof buf);//把"电脑看的数字地址"转换成"人看的文字地址"
    return buf;
}

std::string InetAddress::toIpPort() const{
    //ip:port
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end = ::strlen(buf);
    uint16_t port = ::ntohs(addr_.sin_port);
    sprintf(buf+end,":%u",port);//%u 表示 无符号十进制整数（unsigned int）
    return buf;
}

uint16_t InetAddress::toPort() const{
    return ::ntohs(addr_.sin_port);
}

#if 0 //后面的代码不会编译,需要测试时改成1
#include <iostream>
int main()
{
    InetAddress addr(8080);
    std::cout << addr.toIpPort() << std::endl;
}
#endif