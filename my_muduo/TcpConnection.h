#pragma once
#include "InetAddress.h"
#include "Socket.h"
#include"noncopyable.h"
#include"Callbacks.h"
#include<memory>
#include<string>
#include<atomic>
#include<Buffer.h>
#include"Timestamp.h"
//Acceptor中accept函数拿到connfd，，然后给
//TcpConnection表示一个TCP连接，包含读写操作、事件处理等操作
class Channel;
class EventLoop;
class Socket;

class TcpConnection : noncopyable,public std::enable_shared_from_this<TcpConnection>
{
    public:
        TcpConnection(EventLoop *loop,const std::string &name,int sockfd, const InetAddress &peerAddr, const InetAddress &localAddr);
        ~TcpConnection();
        EventLoop *getLoop() const{return loop_;}
        const std::string &getName() const{return name_;}
        const InetAddress &localAddress() const{return localAddr_;}
        const InetAddress &peerAddress() const{return peerAddr_;}

        bool IsConnected() const{return state_==kConnected;}
        
        void send(const void*message,int len);
        void shutdown();

        void setConnectionCallback(ConnectionCallback cb){connectionCallback_=cb;}
        void setMessageCallback(MessageCallback cb){messageCallback_=cb;}
        void setWriteCompleteCallback(WriteCompleteCallback cb){writeCompleteCallback_=cb;}
        void setCloseCallback(CloseCallback cb){closeCallback_=cb;}
        void setHighWaterMarkCallback(HighWaterMarkCallback cb){highWaterMarkCallback_=cb;}
        void connectEstablished();//连接建立
        void connectDestroyed();//连接销毁

    private:
      enum State{
        kDisconnected,
         kDisconnecting,
        kConnecting,
        kConnected,
    };
     void setState(State state){
            //设置连接状态
            state_.store(state);
        }
        
    //这些是channel的回调函数，用于处理读写事件
        void handleRead(Timestamp receiveTime);
        void handleClose();
        void handleWrite();
        void handleError();

        void send(const std::string &message);
        void sendInLoop(const char*message,int len);
        
        void Shutdown();
        void shutdownInLoop();

        EventLoop *loop_;//这里不是主线程，因为TcpConnection是在subLoop中管理的
        const std::string name_;
        std::atomic<State> state_;
        bool reading_;
//这里与Acceptor类似，一个在mainLoop，一个在subLoop中，都需要channel和socket
        std::unique_ptr<Channel> channel_;
        std::unique_ptr<Socket> socket_;
        const InetAddress peerAddr_;
        const InetAddress localAddr_;

        ConnectionCallback connectionCallback_;//由TcpServer传下来的
        MessageCallback messageCallback_;
        WriteCompleteCallback writeCompleteCallback_;
        CloseCallback closeCallback_;
        HighWaterMarkCallback highWaterMarkCallback_;

        size_t highWaterMark_;
        Buffer inputBuffer_;
        Buffer outputBuffer_;
};
