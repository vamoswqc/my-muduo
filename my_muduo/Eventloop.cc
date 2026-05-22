#include "Eventloop.h"
#include "Channel.h"
#include"Logger.h"
#include"Poller.h"
#include"./getThreadTid/CurrentThread.h"
#include <sys/socket.h>
#include <unistd.h>
#include <sys/eventfd.h>
#include<fcntl.h>
#include<mutex>



//全局变量，构造函数中赋值为this，用于当前线程标识自己这个EventLoop对象，供其他线程访问
thread_local EventLoop* t_loopInThisThread=0;

const int kpollTimesMs=10000;//定义Poller的超时时间，10秒

//创建wakeupFd_，用于mainLoop唤醒subLoop处理新来的channel
int createEventfd()
{//创建一个eventfd对象，初始值为0，非阻塞，关闭时自动关闭文件描述符
    int evtfd=::eventfd(0,EFD_NONBLOCK|EFD_CLOEXEC);
    if(evtfd<0)
    {
        LOG_ERROR("createEventfd() -Failed in eventfd");
        abort();
    }
    return evtfd;
}

EventLoop::EventLoop()
  :looping_(false),
   quit_(false),
   callingPendingFunctors_(false),
   threadId_(CurrentThread::tid()),
   poller_(Poller::newDefaultPoller(this)),
   wakeupFd_(createEventfd()),
   wakeupChannel_(new Channel(this,wakeupFd_))
{
   LOG_DEBUG("EventLoop create %p in thread %d",this,threadId_); 
   if(t_loopInThisThread)  //如果当前线程已经有一个EventLoop对象了
{
    LOG_FATAL("Another EventLoop %p exists in this thread %d",t_loopInThisThread,threadId_);
 } else
 {
    t_loopInThisThread=this; //把当前线程的EventLoop对象赋值给t_loopInThisThread
 }  
    //设置wakeupFd_的事件类型以及发生时的回调函数
     wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this));
    // 每一个eventloop都将监听wakeupChannel_的读事件
     wakeupChannel_->enableReading(); 
     ::close(wakeupFd_);
     t_loopInThisThread=nullptr;
}

EventLoop::~EventLoop()
{
  wakeupChannel_->disableAll(); //禁止wakeupChannel_的所有事件
  wakeupChannel_->remove(); //把wakeupChannel_从poller中删除
}


void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %ld bytes instead of 8",n);
    }
}

void EventLoop::loop(){
    looping_=true;
    quit_=false;
    LOG_INFO("EventLoop %p start looping",this);
 //循环调用Poller的poll函数，等待事件发生，并处理事件   
    while(!quit_){
        activeChannels_.clear();
        pollReturnTime_=poller_->poll(kpollTimesMs,&activeChannels_);
        for(Channel *channel : activeChannels_)
        {//Poller监听哪些channel发生事件上报给eventloop，然后通知channel处理事件
            channel->handleEvent(pollReturnTime_);     
        }
        doPendingFunctors();//执行回调函数列表中的所有函数对象
    }
    LOG_INFO("EventLoop %p stop looping",this);
    looping_=false;
}

void EventLoop::quit()
{//这里设置的是调用者指定的 EventLoop 对象的 quit_
//如果本来就在自己线程中，直接quit_为true后loop循环会退出
//如果是其他线程中调用当前EventLoop->quit()，这就还要调用wakeup（）来让当前线程的poll立即返回然后退出    
    quit_=true;
    if(!isInLoopThread())
    {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor fun){
    if(isInLoopThread()){
        fun();
    }else{//当非当前loop执行fun，
        queueInLoop(fun);
    }
}
//把fun放入队列中，唤醒loop所在的线程执行fun
void EventLoop::queueInLoop(Functor fun)
{
    {//加锁，保护pendingFunctors_这个共享资源
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(fun);
    }
//||callingPendingFunctors_：用于某个回调函数中又调用了queueInLoop(新fun)，此时也要唤醒来执行新fun
    if(!isInLoopThread()||callingPendingFunctors_){
        wakeup();
    }
}

//向wakeupFd_写入一个数据，唤醒loop所在的线程
void EventLoop::wakeup(){
  uint64_t one=1;
  ssize_t n=write(wakeupFd_,&one,sizeof one);
    if(n != sizeof one){
        LOG_ERROR("EventLoop::wakeup()writes %lu bytes instead of 8",n);
    }
}

void EventLoop::updateChannel(Channel *channel){
      poller_->updateChannel(channel);
}  

void EventLoop::removeChannel(Channel *channel){
      poller_->removeChannel(channel); 
}

bool EventLoop::hasChannel(Channel *channel){
   return poller_->hasChannel(channel); 
}

void EventLoop::doPendingFunctors(){
    std::vector<Functor>functors;
    callingPendingFunctors_=true;//正在调用回调函数了
    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);//交换使得取出要执行函数同时pendingFunctors_变成空
    }
    for(const Functor &fun : functors)
    {
        fun();//执行当前loop要执行的回调函数
    }
    callingPendingFunctors_=false;//调用完了回调函数了
}

/*主EventLoop是自己声明，其他工作EventLoop是通过线程池创建的，因此调用这些方法前需要加上所属的EventLoop：：
因此这些操作可以是主线程通过 EventLoop 对象指针访问调用，在当前线程中通过subLoop->XXXXX()调用
*/

/*在doPendingFunctors()中，先将现在加进来的要执行的函数放到一个局部容器中，因为在执行这些函数时，
其他线程可能又会调用queueInLoop()往pendingFunctors_中添加新的函数，因此每次在取出函数swap时都要加锁，取出后
释放锁，执行其中函数时其他线程可以继续投递任务而不会被阻塞。
核心设计思想：
最小化临界区：锁只保护必要的操作
解耦任务获取与执行：执行任务的同时允许其他线程继续投递任务
提高并发性能：其他线程可以在回调执行期间继续投递任务

另外，callingPendingFunctors_ 是一个状态标志，只在特定阶段为 true
它的作用是处理回调期间嵌套投递任务的特殊场景，如果某个回调函数func中正在执行时又调用了queueInLoop(newfunc)，
如果不加上callingPendingFunctors_这个标志，就不会立刻wakeup让poll返回（因为此时只有！isInLoopThread()不满足）
并再次doPendingFunctors()执行newFunc了，而是会超时返回后才执行，这显然不是想要的。
runInLoop 的语义："确保在目标线程立即执行，适合同步操作"
queueInLoop 的语义："安排一个任务在后续执行，适合异步任务调度"
*/

/*thread_local 是 C++11 标准中的关键字，专门用来声明线程局部存储变量
thread_local 修饰的变量是每个线程私有的，无需加锁，天然线程安全。

wakeupFd_是专属于当前线程的，其被封装成wakeupChannel_，专门用于跨线程唤醒。
*/