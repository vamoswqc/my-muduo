#include"Acceptor.h"
#include <netinet/in.h>
#include<sys/types.h>
#include<sys/socket.h>
#include "InetAddress.h"
#include"Logger.h"
#include<unistd.h>
static int createNonblocking(){
    int sockfd=::socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC, 0);
    if(sockfd<0){
        LOG_FATAL("%s:%s:%d listen socket create failed errno:%d \n",__FILE__,__FUNCTION__,__LINE__,errno);
    }
    return sockfd;  
}

//构造函数创建了Socket并把这个fd封装成channel，同时让Socket绑定传入的InetAddress
Acceptor::Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reuseport)
    :loop_(loop),
    acceptSocket_(createNonblocking()),
    acceptChannel_(loop,acceptSocket_.fd()),
    listening_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr);
    //Accepter.listen ，acceptChannel_相当于这个listenfd，当其检测到读事件（新用户连接）时，
    // 调用handleRead回调函数来将新连接的fd封装分发给subloop
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));
    acceptChannel_.enableReading();
}

Acceptor::~Acceptor(){
    acceptChannel_.disableReading();
    acceptChannel_.remove();
}

void Acceptor::listen(){
    listening_=true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
    //这里会依次调用update()、loop_->updateChannel()、poller_->updateChannel()
}

void Acceptor::handleRead(){
    InetAddress peerAddr;
    int connfd=acceptSocket_.accept(&peerAddr);
    if(connfd>=0){
     if(newConnectionCallback_){  
        newConnectionCallback_(connfd,peerAddr);//轮询找到subloop，唤醒分配当前这个新的channel
    }else{
        ::close(connfd);
    }
  }else{
    LOG_ERROR("%s:%s:%d accept failed errno:%d \n",__FILE__,__FUNCTION__,__LINE__,errno);
    //如果accept失败，且errno是EMFILE，说明acceptSocket_的fd数量已经到达最大限制
    if(errno==EMFILE){
        LOG_ERROR("%s:%s:%d sockfd reached max fd-limit errno:%d \n",__FILE__,__FUNCTION__,__LINE__,errno);
    }
}
}
