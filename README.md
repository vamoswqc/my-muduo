---

| \/ | | |
| \ / |\_ _ \_\_| |_ \_ **_
| |\/| | | | |/ _` | | | |/ _ \
 | | | | |_| | (_| | |_| | (_) |
|_| |\_|\__,_|\__,_|\__,_|\_**/

- [muduo网络库简介](#muduo网络库简介)

## muduo网络库简介

muduo网络库是基于reactor事件处理模式的TCP网络库。其采用Reactor模式，将事件处理和业务逻辑分离，实现了高并发的网络编程。

Reactor模式的核心为：Event事件、Reactor反应堆、Demultiplex事件分发器、EventHandler事件处理器。以下是大致流程：

1.事件注册：应用程序将需要监听的事件及对应的事件处理器(EventHandler)注册到Reactor中

2.事件循环：Reactor启动循环，调用Demuliplex等待事件发生

3.事件检测：当事件就绪时，Demultiplex将事件返回给Reactor

4.事件分发：Reactor根据事件类型找到对应的EventHandler，并触发处理

5.事件处理：EventHandler执行具体的业务逻辑，如读取数据、处理消息等。

Muduo的架构以one loop per thread即每个线程都有一个事件循环设计，每个线程都有它自己的Reactor，负责该线程的所有事件。主线程(main reactor)负责监听新的连接，并把accept后的socket封装起来交付给其他线程的 sub reactor处理，之后各个sub reactor负责与该连接的所有读写事件。

业务逻辑与网络层的解耦：muduo是一个网络层框架，已经把网络层面的接受新的连接、收发数据等操作都封装好了，开发者可以只去关注业务逻辑而不必花费大量时间在底层网络通信的细节。
