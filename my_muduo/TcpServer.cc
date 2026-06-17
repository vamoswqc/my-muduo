#include "TcpServer.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include"Logger.h"  
#include<functional>
#include <netinet/in.h>
#include<strings.h>
#include <sys/socket.h>
static EventLoop* CheckLoopNotNull(EventLoop *loop){
    if(loop == nullptr){
        LOG_FATAL("%s:%s:%d mainloop is nullptr",__FILE__,__func__,__LINE__);
    }
    return loop;
}

//这是用户最上层调用的构造函数，用于创建TcpServer对象
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

    TcpServer::~TcpServer(){
        for(auto &item : connections_){
           //这个局部的conn是智能指针，会自动释放new出来的TcpConnection对象
            TcpConnectionPtr conn(item.second);
            item.second.reset();//将智能指针置为空，避免重复销毁
            //销毁连接
            conn->getLoop()->runInLoop(
                std::bind(&TcpConnection::connectDestroyed,conn)
                           );
        }
    }


     void TcpServer::setThreadNum(int numThreads){
        threadPool_->setThreadNum(numThreads);//设置线程池中IO线程数量
    }

   void TcpServer::start(){
   if(started_++ ==0){
        threadPool_->start();//启动线程池中的IO线程并返回给threadPool管理
        loop_->runInLoop(std::bind(&Acceptor::listen,acceptor_.get()));//  //get这里是智能指针的方法
        // loop_是主线程，这里相当于执行acceptor_->listen();开启监听并更新到主Poller里面
    }
}

    //也就是TcpConnection中的newConnectionCallback_
    void TcpServer::newConnectionHandle(int sockfd,const InetAddress& peerAddr)
    {
        //这个函数传给Acceptor，其调用这个函数轮询分发新连接给subloop
        EventLoop *ioLoop = threadPool_->getNextLoop();//从线程池中获取一个IO线程
        char buf[64]={0};
        snprintf(buf,sizeof(buf),"-%s#%d",ipPort_.c_str(),nextConnectionId_);//生成连接ID
        ++nextConnectionId_;
        std::string connName = name_ + buf;
        LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n",
            name_.c_str(),connName.c_str(),peerAddr.toIpPort().c_str());
            //通过sockfd获取本地ip地址和端口号
            sockaddr_in local;
            ::bzero(&local,sizeof(local));//清零local
            socklen_t addrlen = sizeof(local); 
            if(::getsockname(sockfd,(sockaddr*) &local, &addrlen) == -1){
                LOG_ERROR("TcpServer::newConnectionHandle getsockname failed");
                return;
            }
            InetAddress localAddr(local);
            //创建TcpConnection建立与subloop的连接
            TcpConnectionPtr conn(new TcpConnection(ioLoop,connName,sockfd,localAddr,peerAddr));
            connections_[connName] = conn;//将TcpConnection添加到连接映射表中，对用户可见
            //下面的回调是用户设置给TcpServer然后TcpConnection然后调用的回调
            conn->setConnectionCallback(connectionCallback_);
            conn->setMessageCallback(messageCallback_);
            conn->setWriteCompleteCallback(writeCompleteCallback_);
            //设置了TcpConnection的关闭回调函数，用于在连接关闭时调用用户设置的回调函数
            conn->setCloseCallback(
                std::bind(&TcpServer::removeConnection,this,std::placeholders::_1));
            ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished,conn));
            //在选中的subloop中直接调用连接建立回调函数，通知TcpServer有新连接了
            ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished,conn));

    }

    void TcpServer::removeConnection(const TcpConnectionPtr &conn){
        loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop,this,conn));
    }

    void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn){
        LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection [%s]",
            name_.c_str(),conn->getName().c_str());
        connections_.erase(conn->getName());//从连接映射表中删除
        EventLoop *ioLoop = conn->getLoop();//获取TcpConnection所属的IO线程
        ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed,conn));
    }

   
