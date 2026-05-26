#include"EventLoopThreadPoll.h"
#include"EventLoopThread.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop,const std::string &name)
    :baseLoop_(baseLoop),
     name_(name),
     started_(false),
     numThreads_(0),
     next_(0){
}

EventLoopThreadPool::~EventLoopThreadPool(){
    //析构函数中不需要做什么特殊的清理工作，因为线程对象和EventLoop对象都是通过智能指针管理的，会自动释放资源
}

void EventLoopThreadPool::start(const ThreadInitCallback &cb){
    started_=true;
    for(int i=0;i<numThreads_;++i){
        std::string threadName = name_ + "-" + std::to_string(i);//为每个线程生成一个唯一的名字，格式为"线程池名字-线程编号"
        EventLoopThread *t = new EventLoopThread(cb,threadName);//创建一个新的EventLoopThread对象，传入初始化回调函数和线程名字
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));//将新创建的EventLoopThread对象添加到threads_向量中
        loops_.push_back(t->startLoop());//启动线程的同时，将其添加到loops_向量中
    }
//这样新创建的EventLoopThread以及其中的EventLoop都交到当前线程池的容器中管理了，用户就可以通过线程池来访问这些

    //如果没有设置线程数，for会跳过，到这里直接调用回调函数来初始化EventLoop对象
    if(numThreads_==0&&cb){
        cb(baseLoop_);
    }
}

EventLoop* EventLoopThreadPool::getNextLoop(){
    EventLoop* loop=baseLoop_;//默认返回baseloop
    if(!loops_.empty()){
    loop=loops_[next_];//从loops_向量中获取下一个EventLoop对象
    ++next_;//更新下一个EventLoop对象的索引
    if(next_>=loops_.size()){next_=0;} //轮询
    }
    return loop;
}