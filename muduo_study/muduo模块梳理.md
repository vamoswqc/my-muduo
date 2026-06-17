

~~~
┌─────────────────────────────────────────────────────────────────────┐
│                        mainLoop (主线程)                            │
│  ┌──────────────┐                                                  │
│  │   Acceptor   │  ← 监听新连接，创建 TcpConnection                 │
│  └──────┬───────┘                                                  │
│         │                                                          │
│         ▼                                                          │
│  ┌─────────────────────────────────────────────────────────┐       │
│  │              EventLoopThreadPool                         │       │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐    │       │
│  │  │ subLoop0 │  │ subLoop1 │  │ subLoop2 │  │ subLoop3 │    │       │
│  │  │  (IO线程) │  │  (IO线程) │  │  (IO线程) │  │  (IO线程) │    │       │
│  │  └────┬────┘  └────┬────┘  └────┬────┘  └────┬────┘    │       │
│  │       │            │            │            │           │       │
│  │       ▼            ▼            ▼            ▼           │       │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐    │       │
│  │  │Channel  │  │Channel  │  │Channel  │  │Channel  │    │       │
│  │  │ conn0   │  │ conn1   │  │ conn2   │  │ conn3   │    │       │
│  │  │(fd=5)   │  │(fd=6)   │  │(fd=7)   │  │(fd=8)   │    │       │
│  │  └─────────┘  └─────────┘  └─────────┘  └─────────┘    │       │
│  └─────────────────────────────────────────────────────────┘       │
└─────────────────────────────────────────────────────────────────────┘
~~~



## 事件循环的创建流程

阶段 操作 说明

1. 创建线程池 EventLoopThreadPool::start() -- 创建多个工作线程，每个线程拥有独立的 EventLoop
2. 主线程监听 mainLoop.accept() -- 主线程的 EventLoop 监听 listenfd
3. 连接分配 round-robin 选择工作线程 -- 将新连接的 fd 分配给某个工作线程的 EventLoop
4. 跨线程通知 EventLoop::queueInLoop() -- 通过 wakeup fd 唤醒目标线程处理新连接

## 主线程与工作线程梳理

主线程职责**核心任务**

| 职责           | 说明                                            |
| -------------- | ----------------------------------------------- |
| **接受新连接** | 监听 `listenfd`，调用 `accept()` 接受客户端连接 |
| **连接分配**   | 将新连接分配给工作线程（round-robin 策略）      |
| **管理线程池** | 创建/销毁工作线程，维护线程池状态               |
| **优雅关闭**   | 协调所有工作线程的退出顺序                      |

### 工作线程职责

**核心任务**

| 职责             | 说明                                          |
| ---------------- | --------------------------------------------- |
| **处理 IO 事件** | 监听分配给自己的 `connfd`，处理客户端数据读写 |
| **执行回调函数** | 执行通过 `queueInLoop()` 投递的任务           |
| **维护连接状态** | 管理客户端连接的生命周期                      |

1.主线程给工作线程分配连接：

```
// 主线程接受连接后
int connfd = accept(listenfd, ...);

// 选择一个工作线程（round-robin）
EventLoop* subLoop = threadPool.getNextLoop();

// 跨线程投递任务
subLoop->queueInLoop([connfd, subLoop]{
    // 在工作线程中创建 Channel
    Channel* channel = new Channel(subLoop, connfd);
    channel->setReadCallback(...);
    channel->enableReading();
});

// 唤醒工作线程
subLoop->wakeup();
```

2.工作线程给主线程异常通知

~~~c++
// 工作线程检测到连接异常
void handleError() {
    // 通知主线程移除该连接
    mainLoop->queueInLoop([this]{
        removeConnection(this);
    });
}
~~~

| 对比维度      | 主线程               | 工作线程                   |
| ------------- | -------------------- | -------------------------- |
| **数量**      | 1个                  | 可配置（通常 = CPU核心数） |
| **职责**      | 接受连接、管理线程池 | 处理客户端 IO              |
| **监听 fd**   | listenfd             | connfd                     |
| **EventLoop** | mainLoop             | subLoop                    |
| **生命周期**  | 与进程同寿           | 由线程池管理               |



# channel与TcpConnection

这是一个非常好的架构设计问题！关键在于 **Channel 是一个通用的、可复用的 IO 事件通知器，它不应该知道任何 TCP 业务的细节**。

------

## Channel 被用在三个完全不同的场景

看看你的代码，同一个 Channel 类被三种不同角色使用：

![img](https://vscode-remote+wsl-002bubuntu-002d24-002e04.vscode-resource.vscode-cdn.net/home/wanqicheng/.vscode-server/extensions/marscode.marscode-extension-1.6.35/resource/images/languageIcon/plaintext.svg)

plainText







```
                    ┌─────────────────────────────────┐
                    │         Channel 类               │
                    │  - fd (文件描述符)               │
                    │  - events / revents              │
                    │  - 回调函数 (read/write/close/error) │
                    │  - handleEvent() 事件分发        │
                    └──────────┬──────────────────────┘
                               │
            ┌──────────────────┼──────────────────┐
            ▼                  ▼                  ▼
    ┌──────────────┐  ┌──────────────┐  ┌──────────────┐
    │  Acceptor    │  │  EventLoop   │  │TcpConnection │
    │  (listenfd)  │  │ (wakeupFd)   │  │  (connfd)    │
    └──────────────┘  └──────────────┘  └──────────────┘
       回调里做的事:      回调里做的事:      回调里做的事:
       accept()          read(8字节)        read → inputBuffer_
       拿到新connfd      消费唤醒信号        → messageCallback_
```

**同一个 Channel，三种 fd，三种完全不同的回调逻辑。** 如果把 TCP 的读写逻辑写死在 Channel 里，那 Channel 就不通用了。

------

## 如果把 handleWrite 放在 Channel 里会怎样？

反证一下，假设 Channel 要自己处理写事件：

![img](https://vscode-remote+wsl-002bubuntu-002d24-002e04.vscode-resource.vscode-cdn.net/home/wanqicheng/.vscode-server/extensions/marscode.marscode-extension-1.6.35/resource/images/languageIcon/cpp.svg)

cpp







```
// ❌ 糟糕的设计：Channel 被迫拥有 TCP 相关的所有东西
class Channel {
    Buffer outputBuffer_;    // Channel 不应该知道 Buffer
    Buffer inputBuffer_;
    MessageCallback msgCb_;  // Channel 不应该知道业务回调
    WriteCompleteCallback wcCb_;
    // ... Channel 变成了一个大杂烩
};
```

问题：

| 问题                | 说明                                                         |
| ------------------- | ------------------------------------------------------------ |
| **Acceptor 用不到** | Acceptor 的 listenfd 不需要 inputBuffer/outputBuffer，这些对它毫无意义 |
| **wakeupFd 用不到** | EventLoop 的 wakeupFd 只需要读 8 字节消费掉，不需要任何 Buffer |
| **违反单一职责**    | Channel 变成了"IO 通知 + TCP 处理"的混合体                   |

------

## 正确的分工

![img](https://vscode-remote+wsl-002bubuntu-002d24-002e04.vscode-resource.vscode-cdn.net/home/wanqicheng/.vscode-server/extensions/marscode.marscode-extension-1.6.35/resource/images/languageIcon/plaintext.svg)

plainText







```
Channel 的职责（底层）              TcpConnection 的职责（业务层）
─────────────────────────         ─────────────────────────────
"fd 可读了，通知你"                → handleRead() 从 fd 读到 inputBuffer_
                                   → 触发 messageCallback_ 给用户

"fd 可写了，通知你"                → handleWrite() 从 outputBuffer_ 写到 fd
                                   → 写完了 disableWriting + writeCompleteCallback_

"fd 关闭了，通知你"                → handleClose() 清理资源 + 通知上层

"fd 出错了，通知你"                → handleError() 打印错误日志
```

**Channel 只负责说"发生了什么"，TcpConnection 负责"该怎么处理"**。这就是回调机制的精髓——Channel 在构造时把回调设成了 TcpConnection 的方法：

```
// TcpConnection 构造函数中
channel_->setReadCallback(
    std::bind(&TcpConnection::handleRead, this, std::placeholders::_1)
);
channel_->setWriteCallback(
    std::bind(&TcpConnection::handleWrite, this)
);
```

------

## 一句话总结



**Channel 是"邮递员"，它只负责送信（通知事件）；TcpConnection 是"收信人"，它知道信的内容该怎么处理（读 Buffer、调用户回调）。如果把收信人的逻辑写在邮递员身上，那邮递员就什么事都干不了了。**
