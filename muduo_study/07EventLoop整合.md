# EventLoop类

EventLoop相当于主反应堆
muduo 采用的是经典的 One Loop Per Thread 模型，其成员变量以及方法设计原则都是基于下面这些：

## 线程绑定原则

1. 线程ID绑定

```cpp
class EventLoop : public noncopyable {
private:
    const pid_t threadId_;  // 记录当前loop所在线程ID
    // ...
};
```

- threadId\_ 在构造时被赋值为当前线程的ID
- EventLoop 的事件循环 (loop()) 只能在创建它的线程中运行
- 这是 muduo 线程模型的基石：一个线程对应一个 EventLoop

2. 单线程保证执行

```cpp
// 典型实现逻辑（根据muduo源码推断）
EventLoop::EventLoop()
    : threadId_(CurrentThread::tid())  // 获取当前线程ID
{}
void EventLoop::loop() {
    assert(isInLoopThread());  // 断言：必须在创建loop的线程中调用
    looping_.store(true);
    while (!quit_.load()) {
        poller_->poll(&channels_);
        // 处理活跃事件...
    }
}
```

## 多Loop模型

时间线 →
主线程Loop: poll() → 检测到listenfd可读 → **accept()** → 获取connfd
↓
将connfd封装为Channel，通过queueInLoop()发送到工作线程
↓
继续poll()等待下一个事件

工作线程Loop1: poll()阻塞中 ←── **被wakeup fd唤醒**
↓
执行pendingFunctors中的任务（注册新Channel）
↓
poll()继续监听connfd事件
↓
检测到connfd可读 → 调用Channel的readCallback\_

## （1）单个EventLoop的生命周期

用户注册事件
↓
Channel::enableXxx() → Channel::update()
↓
EventLoop::updateChannel()
新channel都加入channels\_ Map中
↓
EpollPoller::update()
↓
根据channel的index来判断是添加还是删除在epoll中
epoll_ctl(ADD/MOD/DEL) ← 内核注册

┌──────────────────────────────────────────────────────────────────┐
│ EventLoop::loop() 主循环 │
├──────────────────────────────────────────────────────────────────┤
│ ┌────────────────────────────────────────────────────────────┐ │
│ │ 1. EpollPoller::poll() → epoll*wait() 阻塞等待事件 │ │
│ └───────────────────────────┬────────────────────────────────┘ │
│ ↓ 有事件发生（或被wakeup唤醒） │
│ ┌────────────────────────────────────────────────────────────┐ │
│ │ 2. fillActiveChannels() 收集活跃的 Channel │ │
│ └───────────────────────────┬────────────────────────────────┘ │
│ ↓ │
│ ┌────────────────────────────────────────────────────────────┐ │
│ │ 3. 遍历 activeChannels → Channel::handleEvent() │ │
│ │ ↓ │ │
│ │ 调用各回调：readCallback*/writeCallback*/closeCallback*...│ │
│ └────────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────────┘

## （2）与多线程的交互

1. 每个线程都有这样一个循环：
   主线程（Main Thread） 工作线程（Worker Threads）
   ┌─────────────────────┐ ┌─────────────────────┐
   │ EventLoop::loop() │ │ EventLoop::loop() │
   │ ├─ poll() 阻塞 │ │ poll() 阻塞 │  
   │ ├─ 处理listenfd │ │ 处理connfd │  
   │ └─ 分配连接 │ │ 执行回调 │
   └─────────────────────┘ └─────────────────────┘
   │ ▲
   │ accept() 新连接 │
   │ 将connfd分配给工作线程 │
   └────────────────────────────────────┘
   通过 wakeup() 唤醒目标线程

2. wakeup 如何融入这个流程
   流程图中的 `阻塞等待` 环节会被跨线程通信打断：
   · 新连接到来 主线程调用 wakeup() epoll_wait() 被唤醒
   · 跨线程任务 其他线程调用 queueInLoop() epoll_wait() 被唤醒
   · IO事件就绪 内核触发 epoll_wait() 返回事件
