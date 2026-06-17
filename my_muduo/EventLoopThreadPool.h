#pragma once
#include"noncopyable.h"
#include<functional>
#include<string>
#include<vector>
#include<memory>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool:noncopyable{
    public:
        using ThreadInitCallback=std::function<void(EventLoop*)>;

        EventLoopThreadPool(EventLoop *baseLoop,const std::string &name);
        ~EventLoopThreadPool();

        void setThreadNum(int numThreads){
            numThreads_=numThreads;
        }

        void start(const ThreadInitCallback &cb=ThreadInitCallback());
        //如果工作在多线程中，baseloop——默认以轮询方式分配EventLoop对象的指针
        EventLoop* getNextLoop();
        std::vector<EventLoop*> getAllLoops();
        bool started() const{     return started_;}
        const std::string& name() const{return name_;}

    private:
        EventLoop *baseLoop_;
        std::string name_;
        bool started_;
        int numThreads_;
        int next_;
        //线程池中包含的线程对象和对应的EventLoop对象的指针
        std::vector<std::unique_ptr<EventLoopThread>> threads_;
        std::vector<EventLoop*> loops_;
};
