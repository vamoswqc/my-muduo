#pragma once
#include "Eventloop.h"
#include"event.h"
#include"Channel.h"
#include"Logger.h"
#include "noncopyable.h"
#include<vector>
#include<unordered_map>
#include<Timestamp.h>
//Poller相当于多路复用机制，是用的epoll结合channel对其的封装
class Poller:noncopyable{
    public:
        using ChannelList=std::vector<Channel*>;

        Poller(EventLoop* loop);
        virtual ~Poller();
    //给所有IO复用保留统一的接口，在派生类中实现
        virtual Timestamp poll(int timeout,ChannelList* activeChannels)=0;
        virtual void updateChannel(Channel* channel)=0;//channel中传的是this指针
        virtual void removeChannel(Channel* channel)=0;
    //判断channel是否在当前Poller中
        bool hasChannel(Channel* channel)const ;
    //Eventloop可以通过该接口获取默认的IO复用的具体实现
        static Poller* newDefaultPoller(EventLoop* loop);

    protected:
    //使用unordered_map来标识fd和对应的Channel
        using ChannelMap =std::unordered_map<int,Channel*>;
        ChannelMap channels_;
    private:
        EventLoop* ownerLoop_;
};