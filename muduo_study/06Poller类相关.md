# Poller

## 概述

`Poller` 是 muduo 网络库中 **IO 复用机制的抽象层**，它封装了不同的多路复用技术（如 epoll、poll、select），为上层提供统一的接口。`EpollPoller` 是其基于 Linux epoll 实现的具体子类。

在poller类中只有一个存当前所有的channel的channels\_ Map，和一个标识当前所属的EventLoop的指针。其他是一些在抽象基类实现的纯虚函数，因为每个具体的多路复用技术比如epoll、poll等实现都有自己的逻辑。

## EpollPoller子类

EpollPoller中是对epoll的封装，因此有一个epollFd*作为监听仓库，还有vector<epoll_event> events*作为保存返回的事件容器。对于其中的方法函数：

1.`updateChannel()`会先把新channel加入到当前总的channels map中（删除则修改index），而对于epoll中的，会调用每个channel的`isNonEvent()`这种来判断有无感兴趣事件，是添加还是删除在epoll中（即`update函数`）。因为epoll机制中，每个channel的index不同也就导致epoll监听还是删除。

2.`removeChannel()`会从channels map中删除指定的channel，同时从epoll中删除。

3.`poll()`调用epoll机制中的epoll*wait()函数（要监听的fd已经被加入epollFd*中），返回结果存在events\*中（必要时会申请新的内存）。

4.`fillActiveChannels()`会遍历events\_，由于events\_中都是epoll_event类型的，里面的date.fd和date.ptr指向的channel都是绑定的，所以直接将date.ptr指向的channel加入到activeChannels中即可。后续在EventLoop::loop()中遍历activeChannels，调用Channel::handleEvent()处理事件。

### updateChannel

                ┌─────────────┐
                │   kNew      │
                │  (未注册)   │
                └──────┬──────┘
                       │ 加到 channels map
                       ▼
                ┌─────────────┐     ┌─────────────┐
                │   kAdded    │────?│  kDeleted   │
                │  (已注册)   │     │  (已删除)   │
                └──────┬──────┘     └──────┬──────┘
                       │                   │
               ┌───────┴───────┐           │
               ▼               ▼           │
        isNonEvent()?    有事件变更        │
               │               │           │
          是───┴───否          │           │
           ▼                   ▼           │
     EPOLL_CTL_DEL       EPOLL_CTL_MOD     │
           │                               │
           └───────────────┬───────────────┘
                           ▼
                     updateChannel()

---

## Channel 与 Poller 的协作

具体见 07EventLoop类相关.md

---

### 精髓总结

poller类结合epollpoller类将epoll机制封装，每个eventloop只有一个poller，用于监听所有channel，根据epoll_event的结构特性能做到把channel和epoll监听的事件绑定起来?。
绑定机制：epoll_event.data.ptr，
注册时绑定：

```cpp
 struct epoll_event event;
    event.events = channel->events();
    event.data.ptr = channel;   epoll_event
    ::epoll_ctl(epollFd_, operation, fd, &event);
```

而事件触发时直接通过`Channel* channel = static_cast<Channel*>(events_[i].data.ptr);`取回。

这样就将事件channel与IO多路复用机制联系了起来，而他们都在EventLoop类中协作。

## POller类中的newDefaultPoller方法为什么要单独创建一个文件实现

首先DefaultPoller::newDefaultPoller() 是一个工厂方法，用于：

1. 动态选择IO复用实现：根据环境变量 MUDUO_USE_POLL 决定使用哪种IO复用方式
2. 解耦架构：EventLoop不需要直接依赖具体的epoll或poll实现，通过工厂方法获取.但本项目中只实现了epoll，所以默认返回EpollPoller。

这是一个非常经典的设计模式和编译依赖管理问题，体现了依赖倒置原则：**抽象不依赖于具体，具体依赖于抽象**。主要有以下几个关键原因：

1. 避免循环依赖（Circular Dependency）
   Poller 是一个抽象基类，它有具体的派生类。如果把 newDefaultPoller 放在 Poller.cc 中实现：

- Poller.cc 需要包含具体子类的头文件（#include "EPollPoller.h"），而EPollPoller.h 又需要继承 Poller，包含 #include "Poller.h"，这就形成了循环依赖，导致编译失败

2. 工厂模式的正确实现
   newDefaultPoller 是一个典型的工厂方法，它需要知道具体产品类（EPollPoller、PollPoller），但抽象基类不应该依赖具体派生类。
   ┌─────────────────┐
   │ Poller.h │ ← 抽象接口（纯虚函数）
   │ (抽象基类) │
   └────────┬────────┘
   │ 继承
   ┌────┴────┐
   ▼ ▼
   ┌─────────┐ ┌─────────┐
   │EPollPoller│ │PollPoller│ ← 具体实现类
   └────┬────┘ └────┬────┘
   │ │
   └─────┬─────┘
   ▼
   ┌──────────────────┐
   │ DefaultPoller.cc │ ← 工厂方法，负责创建具体实例
   └──────────────────┘

#

┌─────────────────────────────────────────────────────────────────────┐
│ EventLoop │
│ ┌─────────────────────────────────────────────────────────────┐ │
│ │ Poller (EpollPoller) │ │
│ │ ┌─────────────────────────────────────────────────────┐ │ │
│ │ │ channels*: { fd1: Channel*, fd2: Channel*, ... } │ │ │
│ │ │ │ │ │
│ │ │ epollFd* ──? 内核 epoll 实例 │ │ │
│ │ │ │ │ │ │
│ │ │ └──? epoll*event[] │ │ │
│ │ │ ├── event[0].data.ptr = Channel1* │ │ │
│ │ │ ├── event[1].data.ptr = Channel2* │ │ │
│ │ │ └── ... │ │ │
│ │ └─────────────────────────────────────────────────────┘ │ │
│ └─────────────────────────────────────────────────────────────┘ │
│ ▲ │
│ ┌─────────────────┼─────────────────┐ │
│ ▼ ▼ ▼ │
│ Channel1 Channel2 Channel3 │
│ (fd=socket1) (fd=socket2) (fd=socket3) │
│ events*=EPOLLIN events*=EPOLLIN events*=EPOLLOUT │
│ |POLLOUT │
└─────────────────────────────────────────────────────────────────────┘
