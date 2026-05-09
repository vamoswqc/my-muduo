# InetAddress类的存在意义

到这一步，我们去观察TcpServer类的实现，发现TcpServer类中有EventLoop和InetAddress这两个类，因此先来看InetAddress类。

InetAddress类是对网络地址结构体sockaddr_in进行封装的一个类，它的存在意义主要有以下几点：

1. InetAddress类提供了一个更高层次的抽象，使得我们不需要直接操作底层的sockaddr_in结构体，可以通过更友好的接口来创建和管理网络地址。
2. InetAddress类提供了一些方便的成员函数，如toIpPort()、toIp()、toPort()等，使得我们可以更方便地将网络地址转换为字符串表示。

InetAddress类本质上就是一个sockaddr*in结构体的封装，它提供了一些方便的成员函数来操作网络地址。其只有一个sockaddr_in型成员变量addr*，有两种构造函数：

1. 默认构造函数，需要指定IP地址和端口号，默认IP地址为本地回环地址（127.0.0.1）。
2. 或直接通过一个sockaddr_in结构体拷贝构造。

然后其方法有如下几种（具体的见InetAddress.cc）：

1. toIp()：将addr\_中的IP地址转换为字符串表示，返回字符串表示。
2. toPort()：将网络字节序的端口号转换为主机字节序的端口号返回。
3. toIpPort()：返回IP地址和端口号一起的字符串表示。
4. getSockAddr()：返回当前sockaddr_in结构体的指针。

```cpp
class InetAddress
{
public:
    explicit InetAddress(uint16_t port , std::string ip = "127.0.0.1") ; //构造函数，默认ip为本地回环地址
    explicit InetAddress(const sockaddr_in& addr) : addr_(addr) {} //构造函数，直接通过sockaddr_in结构体初始化
    //uint16_t是一个unsigned short int（16位无符号整数类型）
    string toIp() const; //返回IP地址的字符串表示
    uint16_t toPort() const; //返回端口号
    string toIpPort() const; //返回IP地址和端口号的字符串表示
    const sockaddr_in* getSockAddr() const{return &addr_;}; //返回sockaddr结构体的指针
private:
    sockaddr_in addr_;
};
```

## 此封装与源码辨析

1、此封装逻辑只支持IPv4地址，使用简单的sockaddr\*in结构体。而muduo源码中使用union结构体来支持IPv4和IPv6地址，其他方法也更加丰富。

2、官方是将IP和端口转换都封装在sockets中，并在内部调用inet_pton()等更安全的函数。而本例中直接对addr\*进行操作，没有封装在sockets中。

3、源码是通过 Endian.h 提供了统一的字节序转换函数，提高了代码的一致性。并且使用 inet_pton 和 inet_ntop 替代较老的 inet_addr 和 inet_ntoa，提供了更好的错误检测能力。

此写法虽然简洁明了，直接高效，但是在安全性（比如inet_addr()函数不如 inet_pton() 安全）和功能上有一定缺陷。
