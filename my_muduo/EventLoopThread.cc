#include "EventLoopThread.h"
#include "Eventloop.h"   

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const std::string &name)
    : loop_(nullptr),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this), name),
      mutex_(),
      cond_(),
      callback_(cb) {
}
        
    EventLoopThread::~EventLoopThread(){
        exiting_=true;
        if(loop_!=nullptr){
            loop_->quit();
            thread_.join();
        }
    }
    EventLoop* EventLoopThread::startLoop(){//启动线程，返回线程中EventLoop对象的指针
        thread_.start();//启动底层的新线程

        EventLoop *loop=nullptr;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            while(loop_==nullptr){
                cond_.wait(lock);//等待线程函数threadFunc中的条件变量通知，直到loop_被赋值了才继续执行
            }
            loop=loop_;
        }
        return loop;
    }
    
    //下面这个方法是在单独的新线程里面运行的
    void EventLoopThread::threadFunc(){//线程函数，在新线程中运行EventLoop对象
        EventLoop loop;//在新线程中创建一个EventLoop对象
        if(callback_){
            callback_(&loop);//如果用户传入了回调函数，就调用它来对loop进行一些初始化操作
        }
        {
           std::unique_lock<std::mutex> lock(mutex_);
           loop_= &loop;//将新线程中的EventLoop对象的指针赋值给loop_
           cond_.notify_one();//通知等待的线程，loop_已经初始化完成了
        }
        loop.loop();//在新线程中运行事件循环，处理事件
        std::unique_lock<std::mutex> lock(mutex_);
        loop_=nullptr;//事件循环退出后，将loop_置空，表示线程结束了
    }
    