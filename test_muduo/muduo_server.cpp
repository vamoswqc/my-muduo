/*基于muduo网络库开发服务器程序*/

#include <functional>
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
    {
    }
  }
  ~ChatServer();

private:
  // 专门处理用户连接的回调函数
  void onConnection(const TcpConnectionPtr &conn) {}
  TcpServer
      server_; // TcpServer对象没有默认构造函数，需要在构造函数中初始化参数
  EventLoop *loop_;
};

/*
1、setConnectionCallback 是 TcpServer
类提供的一个方法，用于注册一个回调函数，当有新的客户端连接建立或断开时被调用。

bind(&ChatServer::onConnection, this, placeholders::_1) 是 C++ 的函数绑定机制：

&ChatServer::onConnection：获取类成员函数的地址
this：绑定到当前 ChatServer 对象实例
placeholders::_1：占位符，表示回调函数的第一个参数（即 TcpConnectionPtr&）

当有新的客户端连接建立或断开时，TcpServer 会调用 onConnection 函数，并传递一个
TcpConnectionPtr 参数，表示当前的连接对象.

2、onConnection 函数是一个回调函数，当：

新客户端连接到服务器时或者客户端与服务器断开连接时，该函数会被自动调用，参数
conn 包含了连接的详细信息，如： 连接的状态（已建立/已断开）、客户端的 IP
地址和端口、连接的唯一标识符、发送和接收数据的方法



*/