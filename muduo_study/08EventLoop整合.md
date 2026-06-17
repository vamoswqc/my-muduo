# EventLoop类

EventLoop相当于主反应堆，其中包含了Channel类和Poller类。
muduo 采用的是经典的 One Loop Per Thread 模型，其成员变量以及方法设计原则。

### loop函数

EventLoop视角下的fd管理：

~~~
EventLoop
├── poller_ (EpollPoller)
│   ├── epollFd_          ← epoll 实例
│   └── channels_         ← 所有注册的 fd
│       ├── listenfd → Channel (主线程)
│       ├── connfd1  → Channel (工作线程)
│       ├── connfd2  → Channel (工作线程)
│       └── ...
├── wakeupFd_             ← 跨线程唤醒用
└── wakeupChannel_        ← 封装 wakeupFd_
~~~

loop函数主要调用Poller->poll来监听本线程的fd，其中包括 网络socket fd和wakeupFd，当其他线程需要唤醒当前 EventLoop 时，向这个 wakeupfd 写入数据。loop函数中`channel->handlEvent（）`是处理epoll返回的IO事件，而`doPendingFunctors()`是处理其他线程投递的任务。

**关键**：因为我们在构造函数中已经将wakeup fd的回调函数绑定到`handleRead`这个函数了，所以`handleEvent()` 处理 wakeup fd 时只是**消费唤醒信号**（读取8字节），真正的任务在 `pendingFunctors_` 队列中！

### 跨线程投递是怎样的



~~~
线程A                           线程B (EventLoop线程)
┌─────────────────────────┐       ┌─────────────────┐
│ 线程池中获取subLoop              │                 │         
│ runInLoop(func)         │       │ poll() 阻塞等待  │
│   └─ isInLoopThread() → false   │                 │
│       └─ queueInLoop(func)      │                 │
│           ├─ func → queue       │                 │
│           └─ wakeup()           │                 │
│               │                 │                 │
│               ▼                 │                 │
│           write(wakeupFd) ──────────▶│ poll()返回   │
│                         │          │                    │
│                         │          │ handleEvent(wakeupChannel)│
│                         │          │  └─ read(wakeupFd)消费  │
│                         │          │                         │
│                         │          │ doPendingFunctors()      │
│                         │          │  └─ 执行 task            │
│                         │          │                         │
└─────────────────────────┘          └─────────────────────────┘
~~~

**意思是当A线程向B线程投任务，会唤醒B的wakeupfd改变其状态，B中的epoll_wait检测到后便立即返回，然后doPendingFunctors（）处理任务，仅用作立即返回，任务早就投入，就算不立即返回也会调用**

这样高效可扩展，当批量投递任务，一次唤醒便要处理此时投递的多个任务！

## 多线程

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

```
┌──────────────────────────────────────────────────────────────────┐
│                     EventLoop::loop() 主循环                      │
├──────────────────────────────────────────────────────────────────┤
│  ┌────────────────────────────────────────────────────────────┐  │
│  │ 1. EpollPoller::poll() → epoll_wait() 阻塞等待事件          │  │
│  └───────────────────────────┬────────────────────────────────┘  │
│                              ↓ 有事件发生（或被wakeup唤醒）       │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │ 2. fillActiveChannels() 收集活跃的 Channel                  │  │
│  └───────────────────────────┬────────────────────────────────┘  │
│                              ↓                                  │
│  ┌────────────────────────────────────────────────────────────┐  │
│  │ 3. 遍历 activeChannels → Channel::handleEvent()            │  │
│  │    ↓                                                        │  │
│  │    调用各回调：readCallback_/writeCallback_/closeCallback_...│  │
│  └────────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────────┘
```

## （2）与多线程的交互

1. muduo 的 **one loop per thread**（one loop per thread）模型：

   - **mainLoop**：主线程，负责**接收新连接**
   - **subLoops**：N 个子线程，负责**处理已连接的 IO**
   - 它们之间**不靠消息队列、不靠管道、不靠复杂通信**
   - **只靠：wakeup () + 锁 + pendingFunctors** 完成全部交互

   

2. wakeup 如何融入这个流程

   1. mainLoop 拿到新连接：主线程 accept 到新的客户端连接。
   1.  mainLoop 把「处理连接」的任务发给 subLoop

   mainLoop 不会自己处理，而是**把任务丢给某个 subLoop**：

   ```c++
   subLoop->runInLoop( 把新连接交给你处理 );
   ```

   3. runInLoop 内部就是做两件事，**加锁**，把任务放进 `pendingFunctors_`，**wakeup()** 唤醒 当前subLoop
   4. 唤醒后便从dopendingFunctors中把函数取出来swap然后依次执行

3. wakeup 本质就是：**往 subLoop 自己的 eventfd 写一个 8 字节数据**

   **让 subLoop 从 epoll_wait () 阻塞中立刻醒过来！**
