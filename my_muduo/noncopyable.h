/*noncopyable类用于防止拷贝构造函数和赋值运算符被调用，
所有继承自noncopyable的类都不能被复制或赋值。*/

#pragma once // 防止重复包含头文件

class noncopyable {
public:
    noncopyable() = default; // c++11默认构造函数，在禁用拷贝操作的同时，保持类的默认构造能力
    ~noncopyable() = default; // c++11默认析构函数，在禁用析构操作的同时，保持类的默认析构能力
   protected:          
    noncopyable(const noncopyable&) = delete; //delete声明为删除函数，防止通过拷贝构造创建新对象
    noncopyable& operator=(const noncopyable&) = delete; //同理防止通过赋值运算符创建新对象
};

/*原因：通过禁止复制，明确系统中资源的所有权，避免浅拷贝，并且网络编程中多线程操作频繁，复制可能导致安全问题*/
/*像Eventloop、TcpConnection、Socket等类，都需要禁止复制，因为它们的资源是不能复制的*/
/*具体参见muduo_study中的02第一个类noncopyable*/