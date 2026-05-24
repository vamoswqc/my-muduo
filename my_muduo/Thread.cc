#include "Thread.h"
#include "./getThreadTid/CurrentThread.h"
#include<semaphore.h>

std::atomic_int32_t Thread::numCreated_(0);

Thread::Thread(TreadFunc func,const std::string &name):
    started_(false),
    joined_(false),
    tid_(0),
    func_(std::move(func))
{
    setDefaultName();//根据全局线程数+1赋给当前线程默认名字
}
     
Thread::~Thread(){
    if(started_ && !joined_){ //如果已经开始并且没有被其他线程阻塞等待就可以将其分离自己执行
        thread_->detach();//thread类提供的设置分离线程的方法
    }
}

    void Thread::start(){
        started_=true;
        sem_t sem;
        sem_init(&sem,false,0);
        thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
            tid_=CurrentThread::tid(); //获取线程tid
            sem_post(&sem); //获取到tid之后就可以让主线程继续执行了
            func_(); //开启一个新线程，专门执行线程函数
        }));
        //这里必须阻塞等待获取上面线程的tid之后sem_post(&sem)信号量释放，主线程才能继续执行
        sem_wait(&sem);
    }

    void Thread::join(){
        joined_ = true;
        thread_->join();
    }   

    void Thread::setDefaultName(){
        int num = ++numCreated_;
        if(name_.empty()){
            char buf[32]={0};
            snprintf(buf,sizeof buf,"Thread%d",num);
            name_=buf;
        }
    } 

