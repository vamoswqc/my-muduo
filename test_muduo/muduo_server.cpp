/*基于muduo网络库开发服务器程序
EventLoop：事件循环对象，负责监听和分发事件，如新连接、数据可读等。它是服务器程序的核心，负责管理所有的事件和回调函数。
TcpServer：TCP服务器对象，负责监听指定的IP地址和端口，接受客户端连接，并为每个连接创建一个TcpConnection对象。
服务器程序的基本流程如下：
1、创建一个EventLoop对象，作为服务器程序的事件循环。
2、创建一个TcpServer对象，指定监听的IP地址和端口，以及服务器的名称。
3、注册回调函数：使用setConnectionCallback方法注册一个回调函数，当有新的客户端连接建立或断开时被调用。
4、注册回调函数：使用setMessageCallback方法注册一个回调函数，当有数据可读时被调用。
5、设置服务器的线程数量：使用setThreadNum方法设置服务器的线程数量，默认值为1个I/O线程，3个工作线程。
6、启动服务器：调用TcpServer对象的start方法，开始监听指定的IP地址和端口。
*/

#include <functional> //提供了std::function和std::bind等功能，用于函数对象和回调函数
#include <iostream>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>
#include <string>
using namespace std;
using namespace muduo::net;

// 组合TcpServer和EventLoop，实现服务器程序
class ChatServer {
public:
  ChatServer(EventLoop *loop,               // 事件循环
             const InetAddress &listenAddr, // IP地址+端口号
             const string &nameArg)         // 服务器名称
      : server_(loop, listenAddr, nameArg), loop_(loop) {
    // 给服务器注册用户连接的创建和销毁回调函数
    server_.setConnectionCallback(
        bind(&ChatServer::onConnection, this, placeholders::_1));
    // 给服务器注册用户读写事件回调函数
    server_.setMessageCallback(bind(&ChatServer::onMessage, this,
                                    placeholders::_1, placeholders::_2,
                                    placeholders::_3));
    // 设置服务器的线程数量，muduo库自己分配1个I/O线程，3个工作线程
    server_.setThreadNum(4);
  }
  // 启动服务器，开启事件循环
  void start() { server_.start(); }

  ~ChatServer() {}

private:
  // 专门处理用户连接的回调函数
  void onConnection(const TcpConnectionPtr &conn) {
    if (conn->connected()) {
      cout << conn->peerAddress().toIpPort() << " -> "
           << conn->localAddress().toIpPort() << "is online" << endl;
    } // toIpPort()函数返回的是IP地址和端口号的组合
    else {
      cout << conn->peerAddress().toIpPort() << " -> "
           << conn->localAddress().toIpPort() << "is offline" << endl;
      conn->shutdown(); // 关闭连接
      // loop_->quit();关闭事件循环，一般用于关闭服务器程序
    }
  }
  // 专门处理用户读写事件的回调函数
  void onMessage(const TcpConnectionPtr &conn, // 连接对象
                 Buffer *buf,                  // 数据缓冲区
                 muduo::Timestamp receiveTime) // 接收数据的时间戳
  {
    string msg = buf->retrieveAllAsString();
    cout << "rece data: " << msg << " at " << receiveTime.toString() << endl;
    conn->send("我已收到: " + msg);
  }

  TcpServer
      server_; // TcpServer对象没有默认构造函数，需要在构造函数中初始化参数
  EventLoop *loop_;
};

int main() {
  EventLoop loop;               // 创建事件循环对象
  InetAddress listenAddr(8888); // 创建InetAddress对象，指定监听的端口号
  ChatServer server(&loop, listenAddr,
                    "ChatServer"); // 创建ChatServer对象，传入事件循环和监听地址
  server.start();                  // 启动服务器，开始监听
  loop.loop();                     // 以阻塞方式运行事件循环，等待和处理事件
  return 0;
}

/*核心工作机制：
1、setConnectionCallback 是
TcpServer类提供的一个方法，用于注册一个回调函数，当有新的客户端连接建立或断开时被调用。
当有新的客户端连接建立或断开时，TcpServer 监听到新的连接请求，底层的 Acceptor
接受连接，创建新的 TcpConnection 对象， 并将其添加到服务器的连接列表中。会调用
onConnection 函数，并传递一个TcpConnectionPtr参数，表示当前的连接对象.

TcpServer中的setConnectionCallback()有一个成员变量connectionCallback_，专门存储用户连接的回调函数。
当 TcpServer::newConnection() 创建 TcpConnection对象时，将 connectionCallback_
传递给 这个对象的构造函数 TcpConnection 在构造函数中将 回调函数 存储在成员变量
connectionCallback_ 中 TcpConnection 构造完成后，调用 connectEstablished() 方法
connectEstablished() 内部调用 connectionCallback_(shared_from_this())
这就触发了我们之前注册的 onConnection 回调函数

2、onConnection 函数是一个回调函数，当：
新客户端连接到服务器时或者客户端与服务器断开连接时，该函数会被自动调用，参数
conn 包含了连接的详细信息，如： 连接的状态（已建立/已断开）、客户端的
IP地址和端口、连接的唯一标识符、发送和接收数据的方法
*/