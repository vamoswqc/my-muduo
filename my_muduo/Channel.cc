#include "Channel.h"
#include<sys/epoll.h>
#include"Eventloop.h"
#include"Logger.h"
#include<memory>

const int Channel::kNoneEvent=0;
const int Channel::kReadEvent=EPOLLIN|EPOLLPRI;
const int Channel::kWriteEvent=EPOLLOUT;

Channel::Channel(EventLoop* loop,int fd)
         :loop_(loop),fd_(fd)
         ,events_(0),revents_(0),index_(-1),tied_(false)
{
}

Channel::~Channel()
{
}
//tie一般在Channel的构造函数中调用，用于绑定Channel与一个对象
void Channel::tie(const std::shared_ptr<void>& obj)
{
    tie_ = obj;
    tied_ = true;
}
//当改变channel表示的fd上的事件类型时，需要调用update函数更新Poller中的channel
 void Channel::update()
{
//通过Channel所属的EventLoop调用updateChannel函数来更新channel
    loop_->updateChannel(this);
}

void Channel::remove()
{
    loop_->removeChannel(this);
}
 
void Channel::handleEvent(Timestamp receiveTime){
    std::shared_ptr<void> tieGuard;
    if(tied_) //说明Channel与一个对象绑定了
    {
        tieGuard=tie_.lock();//尝试获取Channel绑定的对象
        if(tieGuard)
        {
        handleEventWithGuard(receiveTime);
        }
    }else{
        handleEventWithGuard(receiveTime);
    }
}
//根据poller返回在revents_中的fd事件类型，调用相应的回调函数
//这里回调函数都仅仅只是调用一个空壳，具体逻辑由用户实现并绑定到Channel中
void Channel::handleEventWithGuard(Timestamp receiveTime)
{   
    LOG_INFO("channel handleEvent's revents: %d", revents_);    
    if((revents_ & EPOLLHUP )&& !(revents_ & EPOLLIN)){ //说明fd上发生了挂起事件
        if(closeCallback_){
            closeCallback_();
        }
    }
    if(revents_&EPOLLERR){ //说明fd上发生了错误事件
        if(errorCallback_){
            errorCallback_();
        }
    }
    if(revents_&(EPOLLIN | EPOLLPRI)){ //说明fd上发生了读事件
        if(readCallback_){
            readCallback_(receiveTime);
        }
    }
    if(revents_&EPOLLOUT){ //说明fd上发生了写事件
        if(writeCallback_){
            writeCallback_();
        }
    }
}

/*像update,remove这种操作channel的函数，虽然都是属于每个channel自己，但所有channel是在EventLoop中
管理的，即实际的更新操作是通过EventLoop的某个函数，这也就是为什么每个channel都有一个loop_，用于绑定每个
Channel与管理他们的EventLoop。这样每个channel才能在自己的函数中调用loop_->updateChannel(this)类似这种来管理channel。
而如果这类函数直接通过EventLoop来调用，那这样调用的时候必然需要传入channel的指针，因为要知道当前处理的是哪个channel，而传入channel的
指针这个逻辑我们又写在哪里呢，我们是基于事件驱动而Eventloop又不是事件，因此只能由channel自己来调用，只不过统一的逻辑实现在EventLoop中。
muduo网络库是基于事件循环的，每个channel代表一个fd，意思是只有当channel对应的fd发生事件时，才调用对应函数处理channel，
因此只能把这些函数（update,remove）在channel中仅仅声明出来，实际调用EventLoop的函数进行逻辑处理。
*/
