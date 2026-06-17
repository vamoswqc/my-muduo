#pragma once
#include<string>
#include<netinet/in.h> //包含sockaddr_in结构体的头文件
using namespace std;

//封装sockaddr_in结构体
class InetAddress
{
public:
    InetAddress() = default;
    explicit InetAddress(uint16_t port , std::string ip = "127.0.0.1") ; //构造函数，默认ip为本地回环地址
    explicit InetAddress(const sockaddr_in& addr) : addr_(addr) {} //构造函数，直接通过sockaddr_in结构体初始化
    //uint16_t是一个unsigned short int（16位无符号整数类型）

    string toIp() const; //返回IP地址的字符串表示
    uint16_t toPort() const; //返回端口号
    string toIpPort() const; //返回IP地址和端口号的字符串表示

    const sockaddr_in* getSockAddr() const{return &addr_;}; //返回sockaddr结构体的指针
    void setSockAddr(const sockaddr_in & addr){addr_=addr;} //设置sockaddr_in结构体的值
private:
    sockaddr_in addr_;
};

/*
这里addr_是一个sockaddr_in结构体，其内部有sin_family、sin_port、sin_addr等成员变量
*/