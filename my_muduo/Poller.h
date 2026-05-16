#pragma once
#include"Eventloop.h"
#include"Channel.h"
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
/*
channels_ map 的职责：1、记录所有曾注册过的 channel
2、快速查找：通过 fd 快速定位 channel（用于 fillActiveChannels）
3、生命周期管理：由上层（EventLoop）负责 channel 的创建和销毁


*/