#pragma once
#include"noncopyable.h"

class InetAddress;
//封装socket fd相关的系统调用
/*Socket封装的fd里面的方法其实就是调用socket函数，其本质就是个fd，里面包含了bind，listen，accept这些系统调用还有一些其他函数*/

class Socket:noncopyable
{
    public:
        explicit Socket(int sockfd):sockfd_(sockfd){}
        ~Socket();

        int fd() const{return sockfd_;}
        void bindAddress(const InetAddress &localaddr);
        void listen();
        int accept(InetAddress *peeraddr);
        void shutdownWrite();

        void setTcpNoDelay(bool on);//用于禁止Nagle算法，减少数据包的发送延迟
        void setReusePort(bool on);//设置端口复用
        void setReuseAddr(bool on);//设置地址复用
        void setKeepAlive(bool on);//启用TCP保活机制，检测死连接
    private:
        const int sockfd_;
};