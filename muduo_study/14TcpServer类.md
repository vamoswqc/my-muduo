TcpServer是在mainloop里面直接面向用户的，其里面有一个哈希映射表，里面存的所有建立连接的TcpConnection；其还有一个Accteptor，监听新连接并创建新TcpConnection然后就与选中的subloop建立了联系。



~~~c++
用户代码
  │ server.setConnectionCallback(myCallback);   ← 用户设置回调
  ▼
TcpServer::setConnectionCallback(cb)             [TcpServer.h:L45]
  └─ connectionCallback_ = cb;                   ← 存入 TcpServer 的成员
         │  新连接到来时 ↓
         ▼
TcpServer::newConnectionHandle()                 [TcpServer.cc:L57]
  └─ conn->setConnectionCallback(connectionCallback_);  ← 传给 TcpConnection
         │
         ▼
TcpConnection::setConnectionCallback(cb)         [TcpConnection.h:L31]
  └─ connectionCallback_ = cb;                   ← 存入 TcpConnection 的成员
         │
         │  连接建立/销毁时 ↓
         ▼
TcpConnection::connectEstablished()              [TcpConnection.cc:L122]
  └─ connectionCallback_(shared_from_this());    ← 调用的就是用户最初设置的那个！
~~~

同样的模式也适用于 `messageCallback_` 和 `writeCompleteCallback_`——都是 TcpServer 作为"中转站"，把用户设置的回调原封不动地传递给每一个新创建的 TcpConnection。这就是 muduo 的回调传递机制：**用户在 TcpServer 层注册一次，每个 TcpConnection 都能拿到同一份回调逻辑**。



### 开启监听为什么要跨线程开启？

```c++
loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
```

**核心逻辑**：**是在主loop_ 线程**，立即执行listen,不在的话调用`queueInLoop`唤醒

**线程安全考虑**：Acceptor` 的操作（如修改 Channel、注册到 Poller）**不是线程安全的**，必须在 `loop_` 所在线程执行。

| 问题                           | 回答                                                         |
| ------------------------------ | ------------------------------------------------------------ |
| **TcpServer 是否只在主线程？** | 是的，通常只有主线程创建和管理一个 TcpServer                 |
| **为什么还需要 runInLoop？**   | 为了**线程安全**和**代码健壮性**，防止在其他线程调用时出现问题 |
| **实际效果是什么？**           | 如果在主线程调用，`runInLoop` 会直接执行；如果在其他线程，会安全地切换到主线程执行 |

这种设计体现了 muduo 的**防御性编程思想**：即使代码目前在主线程运行，也要考虑未来可能的线程安全问题，提前做好防护。