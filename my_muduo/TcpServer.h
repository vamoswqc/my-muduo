#pragma once
#include"Acceptor.h"
#include"Eventloop.h"
#include"InetAddress.h"
#include"noncopyable.h"
#include <functional>
#include<string>
#include<memory>
#include<EventLoopThreadPool.h>
#include"Callbacks.h"
#include<atomic>
#include<unordered_map>

class EventLoopThreadPool;
/*
 * @brief TcpServer类，用于创建和管理TCP服务器
 * 
 * 这是一个非拷贝可赋值的类，用于创建和管理TCP服务器。
 * 它包含了一个Acceptor对象，用于监听新连接。
 * 还包含了一个EventLoop对象，用于处理IO事件和调用回调函数。
 * 最后，它还包含了一个EventLoopThreadPool对象，用于管理多个线程，每个处理一个连接。
*/

class TcpServer:noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;//因为线程池里创建EventLoopThread对象需要回调函数来初始化EventLoop对象
    enum Option{
        kNoReusePort,//不复用端口
        kReusePort,
    };

    TcpServer(EventLoop *Loop,
        const InetAddress &ListenAddr,
        const std::string &name,
        Option option=kNoReusePort);
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback &cb){threadInitCallback_=cb;}
    void setConnectionCallback(const ConnectionCallback &cb){   connectionCallback_=cb;}
    void setMessageCallback(const MessageCallback &cb){   messageCallback_=cb;}
    void setWriteCompleteCallback(const WriteCompleteCallback &cb){   writeCompleteCallback_=cb;}
    void setThreadNum(int numThreads);
    
    //开启服务器监听
    void start();
private:
    void newConnectionHandle(int sockfd,const InetAddress& peerAddr);
    void removeConnection(const TcpConnection &conn);
    void removeConnectionInLoop(const TcpConnection &conn);
    
    using ConnectionMap=::unordered_map<std::string,std::shared_ptr<TcpConnection>>;//连接映射表
    EventLoop* loop_;//主线程的EventLoop对象

    const std::string name_;
    const std::string ipPort_;
        
    std::unique_ptr<Acceptor> acceptor_;//mainLoop中监听新连接的Acceptor对象
       std::unique_ptr<EventLoopThreadPool> threadPool_;
       
       ConnectionCallback connectionCallback_;//有新连接时的回调
       MessageCallback messageCallback_;//有读写消息时的回调
       WriteCompleteCallback writeCompleteCallback_;//消息写完成时的回调
       
       ThreadInitCallback threadInitCallback_;//线程池初始化时的回调
        std::atomic_int started_;//是否启动了
        int nextConnectionId_;//下一个连接的id
        ConnectionMap connections_;//连接映射表
    };
