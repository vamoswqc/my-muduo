#include "EpollPoller.h"
#include "Channel.h"
#include "Logger.h"
#include <cstring> //memset
#include <sys/epoll.h>
#include <unistd.h>
#include <cerrno>

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

Timestamp EpollPoller::poll(int timeout,ChannelList* activeChannels) {

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
    if(index == kAdded){
        update(EPOLL_CTL_DEL,channel);
    }
    channel->setIndex(kDeleted);
}

void EpollPoller::fillActiveChannels(int numEvents,ChannelList* activeChannels)const{

}    