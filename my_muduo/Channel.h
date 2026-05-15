#pragma once
#include"noncopyable.h"
#include<functional>
#include"Timestamp.h"
#include<memory>
using namespace std;
class EventLoop;

/*Channel类，理解为一个通道，封装了socketfd和它感兴趣的事件，以及发生事件后的回调函数。
每个Channel都属于一个EventLoop，EventLoop负责监听Channel上fd发生的事件，并调用相应的回调函数进行处理。
*/
class Channel: public noncopyable
{
public:
//相当于给函数对象取别名，方便使用。这两个都是函数对象
    using EventCallback=std::function<void()>; 
    using ReadCallback=std::function<void(Timestamp)>;
    Channel(EventLoop* loop,int fd);
    ~Channel();
//fd上发生事件后，EventLoop会调用Channel的handleEvent函数，Channel再调用相应的回调函数进行处理
    void handleEvent(Timestamp receiveTime);
    void setReadCallback(ReadCallback a) {readCallback_=std::move(a);}; //move是将a的资源移动到readCallback_中，避免了拷贝构造
    void setWriteCallback(EventCallback a) {writeCallback_=std::move(a);};
    void setCloseCallback(EventCallback a) {closeCallback_=std::move(a);};
    void setErrorCallback(EventCallback a) {errorCallback_=std::move(a);};
//防止当Channel被手动删除时，EventLoop还在调用Channel的回调函数，
//导致访问野指针，所以当Channel所属的对象被销毁时，Channel也被销毁    
    void tie(const std::shared_ptr<void>&);
    int fd() const {return fd_;};
    int events() const {return events_;};
    void set_revents(int revt) {revents_=revt;};//Poller返回实际发生的事件，此函数提供给Poller调用
    bool isNoneEvent() const {return events_==kNoneEvent;};

//events_是感兴趣的事件，本来应该通过epoll_ctl来注册，但为了提高效率,
//muduo采用了“先修改Channel的events_，再统一调用update()来调用epoll_ctl”的方式
    void enableReading() {events_|=kReadEvent; update();};
    void disableReading() {events_&=~kReadEvent; update();};
    void enableWriting() {events_|=kWriteEvent; update();};
    void disableWriting() {events_&=~kWriteEvent; update();};
    void disableAll() {events_=kNoneEvent; update();};

//bool函数，快捷判断fd是否发生了感兴趣的事件
    bool isNonEvent() const {return events_==kNoneEvent;};
    bool isReading() const {return events_&kReadEvent;};
    bool isWriting() const {return events_&kWriteEvent;};
//反映Channel在Poller中的状态
    int getIndex() {return index_;};
    void setIndex(int idx) {index_=idx;};

//返回Channel所属于哪个EventLoop
    EventLoop* ownerLoop() {  return loop_;}    
    void remove(); 
private:
  void update();
  void handleEventWithGuard(Timestamp receiveTime);
//这些常量是为了标识fd上发生的事件类型，方便在handleEvent函数中判断发生了什么事件
  static const int kNoneEvent;
  static const int kReadEvent;  
  static const int kWriteEvent;

  EventLoop *loop_;//事件循环
  const int fd_;   //fd文件描述符
  int events_;     //感兴趣的事件
  int revents_;    //Poller返回的实际发生的事件
  int index_;//在Poller中的状态
  std::weak_ptr<void> tie_;//当Channel所属的对象被销毁时，Channel也被销毁
  bool tied_;//当tie_不为空时，说明Channel绑定了一个对象，tied_为true，否则为false
 
  //因为channel里面有fd上发生的具体事件revents，所以它可以负责调用具体事件的回调函数
  ReadCallback readCallback_;//声明读事件回调函数，这个函数对象有一个参数，用于传递读取到的时间戳
  EventCallback writeCallback_;//声明写事件回调函数，这个函数对象没有参数，没有返回值，用于处理写事件
  EventCallback closeCallback_;//连接关闭回调函数
  EventCallback errorCallback_;//错误回调函数
};

/*using + std::function = 声明「通用函数对象」类型，它不是声明一个函数
它是声明一种 “能装任何可调用对象” 的容器类型。
using EventCallback = std::function<void()>;是一个 “函数对象类型”。能装：任何 无参、无返回值的函数/可调用对象。
因为 muduo 要实现回调机制：消息来了 → 调用读函数对象、连接来了 → 调用连接函数对象、关闭连接 → 调用关闭函数对象
这些全是函数对象并且要回调，所以muduo必须要这样先把函数对象声明出来，具体业务逻辑留给用户注册回调函数时实现。

std::weak_ptr<void> tie_;是一个弱指针，指向一个void类型的对象。
它的作用是为了防止当Channel被手动删除时，EventLoop还在调用Channel的回调函数，导致访问野指针，
所以当Channel所属的对象被销毁时，Channel也被销毁。
*/