#pragma once
#include "noncopyable.h"
#include"Channel.h"
#include"Socket.h"
#include<functional>
//Acceptor负责在mainloop中接受新的TCP连接
class InetAddress;
class EventLoop;
class Acceptor:noncopyable{
    public:
    using NewConnectionCallback=std::function<void(int sockfd,const InetAddress&)>;
        Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reuseport);
        ~Acceptor();
        void setNewConnectionCallback(const NewConnectionCallback& cb){
            newConnectionCallback_=cb;
        }
        bool listenning()const {return listening_;}
        void listen();
private:
    void handleRead();

    EventLoop* loop_;//用的就是用户定义的那个baseLoop
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;//用户注册的回调函数，mainloop调用这个函数来处理新连接
    bool listening_;
};