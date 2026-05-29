#include "TcpServer.h"
#include"Logger.h"
#include<functional>
EventLoop* CheckLoopNotNull(EventLoop *loop){
    if(loop == nullptr){
        LOG_FATAL("%s:%s:%d mainloop is nullptr",__FILE__,__func__,__LINE__);
    }
    return loop;
}


TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr, const std::string &name, Option option)
    : loop_(CheckLoopNotNull(loop))
    , ipPort_(listenAddr.toIpPort())
    , name_(name)
    , acceptor_(new Acceptor(loop, listenAddr, static_cast<bool>(option)))
    ,threadPool_(new EventLoopThreadPool(loop, name_))
    ,connectionCallback_()
    ,messageCallback_()
    ,nextConnectionId_(1)
    {
    //上层将TcpServer::newConnectionHandle传入acceptor的newConnectionCallback_,
    //accept新连接后，会调用newConnectionCallback_也就是newConnectionHandle来处理新连接
        acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnectionHandle,this
            ,std::placeholders::_1,std::placeholders::_2));
    }

    void TcpServer::newConnectionHandle(int sockfd,const InetAddress& peerAddr)
    {

    }

    void TcpServer::removeConnection(const TcpConnection &conn){

    }

    void TcpServer::setThreadNum(int numThreads){
        threadPool_->setThreadNum(numThreads);//设置线程池中IO线程数量
    }

   void TcpServer::start(){
   if(started_++ ==0){
        threadPool_->start();//启动线程池中的IO线程并返回给threadPool管理
       //get这里是智能指针的
        // loop_是主线程，这里相当于执行acceptor_->listen();开启监听并更新到主Poller里面
    }
}
