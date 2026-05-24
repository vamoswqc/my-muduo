#pragma once
#include<string>
#include"noncopyable.h"
#include<functional>
#include<Thread.h>
#include<mutex>
#include<condition_variable>

/*EventLoopThread类的作用是创建一个新的线程，并在该线程中运行一个EventLoop对象。
它提供了一个接口来获取该线程中的EventLoop对象，以便在其他线程中使用。
EventLoopThread类还提供了一个回调函数，可以在新线程中执行一些初始化操作，例如设置线程名称等。
*/

class EventLoop;

class EventLoopThread:noncopyable
{
    public:
        using ThreadInitCallback= std::function<void(EventLoop*)>;
        
        EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),const std::string &name=std::string());
        ~EventLoopThread();

        EventLoop* startLoop();//启动线程，返回线程中EventLoop对象的指针
    private:
        void threadFunc();//线程函数，在新线程中运行EventLoop对象
        EventLoop *loop_;
        bool exiting_; //标志位，表示线程是否正在退出
        Thread thread_;
        std::mutex mutex_;
        std::condition_variable cond_;//条件变量，用于等待线程初始化完成
        ThreadInitCallback callback_;//线程初始化回调函数

};
/*首先声明一个EventLoopThread类时，构造函数里std::bind 将 threadFunc 绑定为线程执行函数，传递给 Thread 对象。
然后再当前的startLoop()函数中，调用thread_.start() 启动底层线程，底层线程会创建一个EventLoop对象并对其初始化后再将其赋给主线程的loop_,
因此主线程调用 startLoop() 后，会循环检测loop_是否为nullptr，直到loop_被赋值，才会返回loop_。

thread_.start();后说明底层线程已经开始初始化并执行threadFunc()，threadFunc()中会创建一个loop对象，并且会将其地址保存给主线程的loop_
这时候主线程的loop_赋好了就 cond_.notify_one();通知主线程，最后在子线程中开启这个loop的事件循环，等循环退出后，再把主线程的loop_置为空。

当主线程EventLoopThread析构时，如果loop_不为空说明其下面有子线程在执行，因此会loop_->quit()通知 EventLoop退出事件循环，然后thread_.join()等待线程完全结束

*/