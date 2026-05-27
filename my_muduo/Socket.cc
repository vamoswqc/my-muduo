#include"Socket.h"
#include "InetAddress.h"
#include"Logger.h"
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>//提供sockaddr_in结构体定义
#include<netinet/tcp.h> // 提供 TCP_NODELAY
#include<strings.h>//提供bzero函数，用于初始化内存为0


Socket::~Socket(){
    //析构函数中关闭套接字，释放资源
    close(sockfd_);
}

void Socket::bindAddress(const InetAddress &localaddr){
    //绑定套接字到指定的地址和端口
   if( bind(sockfd_,(sockaddr*)(localaddr.getSockAddr()), sizeof(sockaddr_in))!=0){
       LOG_FATAL("bind sockfd:%d failed \n",sockfd_);
   }
}

void Socket::listen(){
    if(0!=::listen(sockfd_,1024)){
        LOG_FATAL("listen sockfd:%d failed \n",sockfd_);
    }       
}

int Socket::accept(InetAddress *peeraddr){
    sockaddr_in addr;
    socklen_t addrlen=sizeof(addr);
    bzero(&addr,sizeof(addr));
    int connfd=::accept(sockfd_,(sockaddr*)&addr,&addrlen);
    if(connfd>=0){
        peeraddr->setSockAddr(addr);
    }
    return connfd;
}

void Socket::shutdownWrite(){
    //关闭写端，防止发送更多数据
    if(::shutdown(sockfd_,SHUT_WR)<0){
        LOG_ERROR("sockets::shutdownWrite error\n");
    }
}

void Socket::setTcpNoDelay(bool on){
    int optval=on ? 1:0;
    ::setsockopt(sockfd_,IPPROTO_TCP,TCP_NODELAY,&optval,sizeof(optval));
}

void Socket::setReusePort(bool on){
    int optval=on ? 1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEPORT,&optval,sizeof(optval));
}

void Socket::setReuseAddr(bool on){
    int optval=on ? 1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
}

void Socket::setKeepAlive(bool on){
    int optval=on ? 1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_KEEPALIVE,&optval,sizeof(optval));
}

/*Socket封装的fd里面的方法其实就是调用socket函数，其本质就是个fd，里面包含了bind，listen，accept这些系统调用还有一些其他函数*/
