#pragma once
#include"noncopyable.h"
#include<functional>
#include<thread>
#include<memory>
#include<unistd.h>
#include<string>
#include<atomic>


class Thread:public noncopyable
{
    public:
    using TreadFunc = std::function<void()>;
    explicit Thread(TreadFunc func,const std::string &name=std::string());
     
    ~Thread();

    void start();
    void join();
     
    bool started() const {return started_;}
    pid_t tid() const {return tid_;} //获取线程tid
    const std::string& name() const{return name_;}
    void setDefaultName();
    static int numCreated() {return numCreated_;} //获取线程数量
    
    
    private:
      void setdefaultname();
      bool started_;
      bool joined_;
      std::shared_ptr<std::thread> thread_;
      pid_t tid_;
      TreadFunc func_;   //线程函数
      std::string name_; //线程名称
      std::atomic<bool> exit_; //线程是否退出
      static std::atomic_int32_t numCreated_; //线程数量     
};