# 目录

- [目录](#目录)
  - [Chatserver](#chatserver)
  - [其封装的设计精髓](#其封装的设计精髓)
  - [运行Chatserver](#运行chatserver)

## Chatserver

Chatserver是将一个EventLoop对象和一个TcpServer对象封装起来，其在构造时与用户定义的连接事件和用户定义的可读事件处理函数绑定，当事件发生时，会通过回调函数onConnection和onMessage来处理事件。

1、TcpServer::TcpServer()
当我们创建一个TcpServer对象，即执行代码TcpServer server(&loop, listenAddr); 调用了TcpServer的构造函数，TcpServer构造函数最主要的就是类的内部实例化了一个Acceptor对象，并往这个Acceptor对象注册了一个回调函数TcpServer::newConnection()

2、Acceptor::Acceptor()
当我们在TcpServer构造函数实例化Acceptor对象时，Acceptor的构造函数中实例化了一个Channel对象，即acceptChannel_，该Channel对象封装了服务器监听套接字文件描述符。
然后Acceptor构造函数将Acceptor::handleRead()方法注册到acceptChannel_中（回调），这也意味着，日后如果事件监听器监听到acceptChannel_发生可读事件，将会调用Acceptor::handleRead()函数。

3、创建Chatserver对象时，自动启动start()方法，开启TcpServer，用于监听新的连接（具体查看TcpServer.cc中的start()方法实现）。

4、Acceptor::handleRead()
程序处理新客户连接请求，acceptChannel_发生可读事件，2中提到会调用Acceptor::handleRead()函数。该函数首先调用了Linux的函数Accept()接受新客户连接。接着调用了TcpServer::newConnection()函数创建了一个TcpConnection对象。

5、TcpServer::newConnection()
上面TcpServer::newConnection()函数创建了一个新的TcpConnection对象，TcpConnection在构造函数中将回调函数存储在成员变量connectionCallback_中，其构造完成后，会调用connectEstablished()方法
connectEstablished()内部调用connectionCallback_(shared_from_this())，这就触发了我们之前注册的onConnection回调函数。

## 其封装的设计精髓

1、其Reactor模式基于事件驱动，通过EventLoop统一管理事件，只有当事件发生时才处理，避免了传统轮询的开销。
2、使用现代C++回调，通过bind绑定this指针，回调函数可以访问类的成员变量和方法，使得只需要再回调函数来处理业务逻辑。而不是使用函数指针，回调函数无法访问类成员变量和方法。
3、One Loop Per Thread（**每个线程一个事件循环**），充分利用多核CPU，并且每个连接由固定线程处理，避免了线程切换的开销。
4、将服务器逻辑封装在ChatServer类中，开发者只需要关注业务逻辑，而不需要关注底层网络通信的细节，添加新功能只需修改回调函数，无需修改网络处理代码。

## 运行Chatserver

1、在终端编译Chatserver

```bash
g++ muduo_server.cpp -o server -lmuduo_net -lmuduo_base -lpthread
```

2、然后在终端运行Chatserver

```bash
./server
```

观察到终端出现127.0.0.1:45590 -> 127.0.0.1:8888 is online，说是Chatserver已经成功启动并监听在8888端口上了。
3、在另一个终端使用telnet连接Chatserver

```bash
telnet localhost 8888
```

4、此时在telnet终端输入任意内容，即可看到Chatserver返回的内容。断开连接按下Ctrl+]即可。  
