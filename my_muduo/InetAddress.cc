#pragma once
#include <cstring>
#include<string>
#include <netinet/in.h> //包含sockaddr_in结构体的定义
#include<strings.h>  //包含bzero函数的头文件
#include "InetAddress.h" //包含InetAddress类的头文件
#include <arpa/inet.h>  //包含inet_addr函数的头文件
using namespace std;
 InetAddress:: InetAddress(uint16_t port , std::string ip ){
   bzero(&addr_ , sizeof(addr_)); //将addr_结构体的内存空间清零，确保结构体中没有残留的垃圾值
   addr_.sin_family = AF_INET; //设置地址族为IPv4
   addr_.sin_port = htons(port); //将端口号转换为网络字节序并存储在sin_port中
   addr_.sin_addr.s_addr = inet_addr(ip.c_str()); 
   //inet_addr函数将IP地址字符串（如 "127.0.0.1"）转换为网络字节序的32位整数
   //因为inet_addr函数的参数是一个char*类型，因此调用string中的c_str()将ststring转换为C风格的字符串
}
//这个构造函数允许用户通过指定IP地址和端口号来创建一个InetAddress对象 
 
 string InetAddress::toIp() const{
     char buffer[64];          //定义一个字符数组来存储转换后的IP地址字符串
     inet_ntop(AF_INET, &addr_.sin_addr, buffer, sizeof(buffer));
     //inet_ntop函数将网络字节序的IP地址转换为点分十进制形式，并将结果存储在buffer中
     return string(buffer); //inet_ntop函数返回char*类型，将其转换为string对象返回
 };//返回IP地址的字符串表示
   
 uint16_t InetAddress::toPort() const{
    return ntohs(addr_.sin_port); //将网络字节序的端口号转换为主机字节序
    }; //返回端口号
    
 string InetAddress::toIpPort() const{
    char buffer[64]; //定义一个字符数组来存储转换后的IP地址和端口号字符串
    inet_ntop(AF_INET, &addr_.sin_addr, buffer, sizeof(buffer)); //转换IP地址
    size_t end=strlen(buffer); //获取IP地址字符串的长度
    uint16_t port=ntohs(addr_.sin_port); //将网络字节序的端口号转换为主机字节序
    sprintf(buffer+end, ":%u", port); //将端口号以字符串形式追加到IP地址字符串的末尾，格式为":端口号"
    return string(buffer); 
 }

 /*测试代码
 #include<iostream>
 int main(){
    InetAddress addr(8080);
    std::cout<<addr.toIpPort()<<std::endl;
    std::cout<<addr.toIp()<<std::endl;
    std::cout<<addr.toPort()<<std::endl;
    return 0;
 } */