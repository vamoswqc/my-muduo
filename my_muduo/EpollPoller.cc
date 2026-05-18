#include "EpollPoller.h"
#include "Channel.h"
#include "Logger.h"
#include <cstring> //memset
#include <sys/epoll.h>
#include <unistd.h>
#include <cerrno>
#include<Timestamp.h>

const int kNew=-1;//表示channel还未添加到epoll中。只是对epoll，并非对channels_ map
const int kAdded=1;//表示channel已添加到epoll中
const int kDeleted=2;//表示channel已从epoll中删除

EpollPoller::EpollPoller(EventLoop* loop):
   Poller(loop)
   ,epollFd_(::epoll_create1(EPOLL_CLOEXEC))
   ,events_(kInitEventListSize)
{
  if(epollFd_<0){
    LOG_FATAL("EpollPoller::EpollPoller() epoll_create1 error:%d", errno);
  }
}

EpollPoller::~EpollPoller(){
    ::close(epollFd_);
}
//轮询epoll中的channel,调用epoll_wait将返回的
Timestamp EpollPoller::poll(int timeout,ChannelList* activeChannels) {
   LOG_DEBUG("func=%s =>fd tatal count:%d",__func__,channels_.size());
   int numEvents=::epoll_wait(epollFd_,&events_[0],static_cast<int>(events_.size()),timeout);
   int saveError=errno;//poll同时被多次调用，保存当前错误码,error记录的全局的错误
   Timestamp now(Timestamp::now());

   if(numEvents<0){
    if(saveError!=EINTR){
        errno = saveError;
        LOG_ERROR("epoll_wait error:%d", saveError);
    }
   }else if(numEvents==0){
    LOG_DEBUG("%s timeout",__func__);
   }else{
    LOG_INFO("%d events happened",numEvents);
    fillActiveChannels(numEvents,activeChannels);
    if(events_.size() == numEvents){
        events_.resize(numEvents*2);
    }
   }
   return now;
}
//根据返回的events_[]的data的ptr，来获取对应的channel并添加到activeChannels中
 void EpollPoller::fillActiveChannels(int numEvents,ChannelList* activeChannels)const{
        for(int i=0;i<numEvents;++i){
         //将void* 类型的 data.ptr安全转换为 Channel*
            Channel *channel=static_cast<Channel*>(events_[i].data.ptr);
            activeChannels->push_back(channel);
 }
}

//根据channel的index来判断是添加还是删除在epoll中
void EpollPoller::updateChannel(Channel* channel){
 const int index=channel->getIndex();
  LOG_INFO("fd=%d,events=%d",channel->fd(),channel->events());
  if(index == kNew||index == kDeleted){
// 如果是新channel，需要先注册到channels_ map中
//如果是deleted过的，则跳过注册
    if(index == kNew){
        int fd=channel->fd();
        channels_[fd]=channel;
    }
        channel->setIndex(kAdded);
        update(EPOLL_CTL_ADD,channel);
    }else{
    //状态为kAdded：channel已经注册到epoll
        int fd=channel->fd();
        if(channel->isNonEvent()){
    //事件为空，只是从epoll中删除该channel
            update(EPOLL_CTL_DEL,channel);
    //标记状态为已删除（但仍保留在channels_中）
            channel->setIndex(kDeleted);
        }else{
    //事件不为空，修改epoll中的监听事件
            update(EPOLL_CTL_MOD,channel);
        }
  }
}


//在updateChannel的过程中调用update函数，来更新epoll中的channel
void EpollPoller::update(int operation,Channel *channel){
  struct epoll_event event;
   int fd=channel->fd();
  memset(&event,0,sizeof(event));
  event.events=channel->events();
  event.data.ptr=channel;
  event.data.fd=fd;
  if(::epoll_ctl(epollFd_,operation,fd,&event)<0){
    if(operation==EPOLL_CTL_DEL){
        LOG_ERROR("epoll_ctl DEL error:%d", errno);
    }else{
        LOG_FATAL("epoll_ctl ADD/MOD error:%d", errno);
    }
  }
}
//从poller中删除channel，epoll和channels_ map中都删除
void EpollPoller::removeChannel(Channel* channel){
    int fd=channel->fd();
    int index=channel->getIndex();
    channels_.erase(fd);

    LOG_INFO("removeChannel fd=%d",fd);
    if(index == kAdded){
        update(EPOLL_CTL_DEL,channel);
    }
    channel->setIndex(kDeleted);
}

  