#pragma once
#include "Timestamp.h"
#include"noncopyable.h"
#include<functional>
#include<vector>
#include<atomic>
#include<memory>
#include<mutex>
//事件循环类，主要包含两大模块：Channel和Poller（epoll的封装）
class Channel;
class Poller;  

class EventLoop:public noncopyable
{
    public:
        using Functor = std::function<void()>;//定义一个无参数无返回值的函数指针类型，用于存储事件处理函数
        
        EventLoop();
        ~EventLoop();

        void loop();//开启事件循环
        void quit();//退出事件循环

        Timestamp pollReturnTime() const{return pollReturnTime_;}
        void runInLoop(Functor fun);//再当前loop线程执行fun
        void queueInLoop(Functor fun);//把fun添加到pendingFunctors_中，唤醒loop所在的线程，执行fun

        void wakeup();//唤醒其他线程，让其立即返回poll函数

        void updateChannel(Channel* channel);
        void removeChannel(Channel* channel);
        bool hasChannel(Channel* channel);
        
        bool isInLoopThread() const{return threadId_==pthread_self();}//判断当前线程是否是loop所在的线程
    private:
        void handleRead();//处理wakeupChannel_的读事件，唤醒事件循环
        void doPendingFunctors();//执行回调函数列表中的所有函数对象
        using ChannelList=std::vector<Channel*>;
        ChannelList channels_;//存储所有注册的Channel指针

        std::atomic_bool looping_;//原子布尔变量，用于控制事件循环的运行
        std::atomic_bool quit_;//原子布尔变量，用于判断是否需要退出事件循环
        
        const pid_t threadId_;//记录当前loop所在线程ID

        Timestamp pollReturnTime_;//记录Poller返回的时间点
        std::unique_ptr<Poller> poller_;//epoll的封装类指针
        
        //当mainLoop获取一个新的用户channel，通过轮询算法选择一个subloop，通过wakeupFd_唤醒subloop
        int wakeupFd_;
        std::unique_ptr<Channel> wakeupChannel_;//用于存储唤醒subloop的channel指针
        
        ChannelList activeChannels_;//存储Poller中poll函数返回的channel指针列表
       
        std::atomic_bool callingPendingFunctors_;//判断是否正在调用pendingFunctors_中的函数对象
        std::vector<Functor> pendingFunctors_;//存储loop需要执行的所有的回调函数
        std::mutex mutex_;//用于保护pendingFunctors_的互斥锁
    };

    /*std::atomic_bool 是 C++11 标准引入的原子布尔类型，定义在 <atomic> 头文件中。
    它提供了原子操作，即在多线程环境下，对该变量的读写操作是不可分割的，不会出现数据竞争
    looping_  |	控制事件循环是否正在运行  |	其他线程可能需要检查循环状态
    quit_  |	标记是否需要退出循环	        |	其他线程调用 quit() 时设置
    callingPendingFunctors_  |	标记是否正在执行待处理函数 |	防止重复调用或递归调用
     */