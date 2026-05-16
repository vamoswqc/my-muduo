#pragma once
#include"Poller.h"
#include<vector>
#include<sys/epoll.h>
#include<Timestamp.h>

class Channel;

class EpollPoller:public Poller{
    public:
        EpollPoller(EventLoop* loop);
        ~EpollPoller() override;
    //重写基类Poller的抽象方法
        Timestamp poll(int timeout,ChannelList* activeChannels) override;
        void updateChannel(Channel* channel) override;//channel中传的是this指针
        void removeChannel(Channel* channel) override;
    private:
        static const int kInitEventListSize=16;//给epoll_event分配初始空间
    //根据epoll_wait返回的事件数，填充activeChannels
        void fillActiveChannels(int numEvents,ChannelList* activeChannels) const;
    //根据channel的fd和操作类型，调用epoll_ctl更新epoll事件
        void update(int peration,Channel *channel);
        
        using EventList = std::vector<struct epoll_event>;
        int epollFd_;
        EventList events_;//虽然事件封装在channel中，但是epoll的相关函数是基于epoll_event的
};

/*在EpollPoller中实现epoll机制，其中：
在构造函数中对应epoll_create，创建epoll文件描述符存在epollFd_中
epoll_wait在poll函数中调用，等待epoll事件发生
而updateChannel和removeChannel则代表epoll_ctl，用于添加或删除epoll事件

关于override关键字：用于显式声明一个成员函数是重写（覆盖）基类中的虚函数。
编译器会检查：基类中是否存在同名同签名的虚函数以确保你确实是在重写，而不是意外地定义了一个新函数
也能更好的做出标识，所有重写虚函数都应该使用 override，这是现代 C++ 的标准做法。
*/