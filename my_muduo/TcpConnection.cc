#include"TcpConnection.h"
#include"Logger.h"
#include"Socket.h"
#include"Channel.h"
#include"Eventloop.h"
#include<functional>
#include<errno.h>
#include<sys/socket.h>
#include<netinet/tcp.h>
#include<strings.h>
#include<string.h>
#include <unistd.h>
static EventLoop* CheckLoopNotNull(EventLoop *loop){
    if(loop == nullptr){
        LOG_FATAL("%s:%s:%d mainloop is nullptr",__FILE__,__func__,__LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop,
        const std::string &name,
        int sockfd,
        const InetAddress &peerAddr,
        const InetAddress &localAddr)
    :loop_(CheckLoopNotNull(loop)),
    name_(name),
    reading_(true),
    state_(kConnecting),
    socket_(new Socket(sockfd)),
    channel_(new Channel(loop,sockfd)),
    peerAddr_(peerAddr),
    localAddr_(localAddr),
    highWaterMark_(64*1024*1024)
{
    //设置channel的回调函数
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead,this,std::placeholders::_1)
    );
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite,this)
    );
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose,this)
    );
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError,this)
    );
    LOG_INFO("TcpConnection::ctor[%s] at fd=%d",name_.c_str(),sockfd);
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection(){
    LOG_INFO("TcpConnection::~dtor[%s] at fd=%d state=%d\n",name_.c_str(),channel_->fd(),state_.load());
}


 void TcpConnection::send(const std::string &message){
    if(state_.load() == kConnected){
        if(loop_->isInLoopThread()){
            sendInLoop(message.c_str(),message.size());
        }else{
            auto self = shared_from_this();
            loop_->runInLoop(
                [this, self, message]() {
                    sendInLoop(message.c_str(), message.size());
                }
            );
        }
    }
}

//内核发送数据慢，需要把待发送数据写入 输出缓冲区中
void TcpConnection::sendInLoop(const char*message,int len){
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    //之前调用过shutdown()关闭连接，需要返回错误
    if(state_.load() == kDisconnected){
        LOG_ERROR("disconnected, give up writing");
        return;
    }
//channel_第一次写事件(第一次够就直接写入内核不经过outbuffer)，且输出缓冲区没有数据，直接将数据写入内核
    if(!channel_->isWriting()&&outputBuffer_.readableBytes() == 0){
        nwrote = ::write(channel_->fd(), message, len);
        if(nwrote >= 0){ //成功写入数据，更新剩余数据长度
            remaining = len - nwrote;
            if(remaining == 0 && writeCompleteCallback_){
                loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
            }
    }else{//nwrote < 0，写入失败，需要记录错误
        nwrote = 0;
        if(errno != EWOULDBLOCK){
            LOG_ERROR("TcpConnection::sendInLoop");
            if(errno == EPIPE || errno == ECONNRESET){ //EPIPE：对端已经关闭连接，ECONNRESET：对端重置连接
                faultError = true;
            }
        }
      }
   }
//说明还有数据没有写入内核，需要把剩余数据写入输出缓冲区中，然后向fd注册EPOLLOUT
   if(!faultError&&remaining > 0){
    //之前没有超过高水位标志，但是加上剩余数据后超过了高水位标志
        size_t oldLen = outputBuffer_.readableBytes();
        if(oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_){
            loop_->queueInLoop(std::bind(highWaterMarkCallback_,shared_from_this(),oldLen + remaining));
        }
        outputBuffer_.append(message + nwrote,remaining);
        if(!channel_->isWriting()){
            channel_->enableWriting();//必须要注册fd的写事件，否则内核不会通知fd何时可写好让poller监听到
        }
   }
}

/*两步构造模式：
构造函数      →  C++ 对象诞生了，但 fd 还没被 epoll 监听
connectEstablished →  fd 开始被 epoll 监听，上层得知"连接可用"
connectDestroyed   →  fd 从 epoll 移除，连接彻底结束
*/
    void TcpConnection::connectEstablished(){
        setState(kConnected);
        channel_->tie(shared_from_this());
        channel_->enableReading();
        //连接建立成功，调用连接回调函数
        connectionCallback_(shared_from_this());
    }

    void TcpConnection::connectDestroyed(){
        if(state_.load() == kConnected){
            setState(kDisconnected);
            channel_->disableAll();
            //连接销毁，调用连接回调函数
            connectionCallback_(shared_from_this());
        }
  }

void TcpConnection::Shutdown(){
    if(state_.load() == kConnected){
        setState(kDisconnecting);
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop,this));
    }
}


void TcpConnection::shutdownInLoop(){
    //sutdownInLoop是用户操作引起的，如果没有写事件，才调用shutdownWrite()关闭写事件
    if(!channel_->isWriting()){
        socket_->shutdownWrite();
    }
}


void TcpConnection::handleRead(Timestamp receiveTime){
    int savedError = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(),&savedError);
    if(n > 0){
        //已经建立连接的用户有可读事件发生，调用传入的消息回调函数
        messageCallback_(shared_from_this(),&inputBuffer_,receiveTime);
    }else if(n == 0){ //对端关闭连接
        handleClose();
    }else{
        errno = savedError;
        handleError();
    }
}

//handleWrite()函数是对接输出缓冲区写到文件描述符fd的函数
void TcpConnection::handleWrite(){
    if(channel_->isWriting()){
        int saveErrno = 0;
        //将缓冲区中的数据写入文件描述符fd
        ssize_t n = outputBuffer_.writeFd(channel_->fd(),&saveErrno);
        if(n>0){
            outputBuffer_.retrieve(n);//将已经写入文件描述符fd的数据从缓冲区中移除
            //如果数据发完了关闭写事件，否则保持EPOLLOUT，epoll继续监听socket内核缓冲区是否可写
            if(outputBuffer_.readableBytes() == 0){
                channel_->disableWriting();
                if(writeCompleteCallback_ ){//调用写完成回调函数
                    auto self=shared_from_this();//引用计数增加1
                    loop_->queueInLoop([this,self]() { //捕获智能指针
                        writeCompleteCallback_(self);  //即使外部释放，对象仍然存活，避免handleWrite()返回，外部又析构释放了对象
                    });
                  }
                if(state_.load() == kDisconnecting){//如果正在关闭连接，执行关闭
                    shutdownInLoop();
                }
            }
        }
        else{  //写入失败
            LOG_ERROR("TcpConnection::handleWrite() errno=%d\n",saveErrno);
        }
    }else{//channel_->isWriting() == false
        LOG_ERROR("TcpConnection fd=%d is down,no write event\n",channel_->fd());
    }
}



void TcpConnection::handleClose(){
    LOG_INFO("fd=%d state=%d \n",channel_->fd(),state_.load());
    setState(kDisconnected);
    channel_->disableAll();
    TcpConnectionPtr self = shared_from_this();
    connectionCallback_(self);//执行连接回调函数，通知TcpServer连接关闭了
    closeCallback_(self);//关闭连接的回调函数
}

void TcpConnection::handleError(){
    int optval;
    socklen_t optlen =sizeof(optval);
    int err=0;
    if(::getsockopt(channel_->fd(),SOL_SOCKET,SO_ERROR,&optval,&optlen) < 0){
        err = errno;//getsockopt()本身失败，直接错误码为errno
    }
    else{ //getsockopt()成功，将获取到的错误码赋值给err
        err = optval;
    }
    LOG_ERROR("TcpConnection handleError name:%s - SO_ERROR:%d \n",name_.c_str(),err);
}