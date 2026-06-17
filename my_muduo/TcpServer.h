#pragma once
#include"Acceptor.h"
#include"Eventloop.h"
#include"InetAddress.h"
#include"noncopyable.h"
#include <functional>
#include<string>
#include<memory>
#include<EventLoopThreadPool.h>
#include"Callbacks.h"
#include<atomic>
#include<unordered_map>

class EventLoopThreadPool;
/*
 * @brief TcpServer�࣬���ڴ����͹���TCP������
 * 
 * ����һ���ǿ����ɸ�ֵ���࣬���ڴ����͹���TCP��������
 * ��������һ��Acceptor�������ڼ��������ӡ�
 * ��������һ��EventLoop�������ڴ���IO�¼��͵��ûص�������
 * �������������һ��EventLoopThreadPool�������ڹ�������̣߳�ÿ������һ�����ӡ�
*/

class TcpServer:noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;//��Ϊ�̳߳��ﴴ��EventLoopThread������Ҫ�ص���������ʼ��EventLoop����
    enum Option{
        kNoReusePort,//�����ö˿�
        kReusePort,
    };

    TcpServer(EventLoop *Loop,
        const InetAddress &ListenAddr,
        const std::string &name,
        Option option=kNoReusePort);
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback &cb){threadInitCallback_=cb;}
    void setConnectionCallback(const ConnectionCallback &cb){   connectionCallback_=cb;}
    void setMessageCallback(const MessageCallback &cb){   messageCallback_=cb;}
    void setWriteCompleteCallback(const WriteCompleteCallback &cb){   writeCompleteCallback_=cb;}
    void setThreadNum(int numThreads);
    
    //��������������
    void start();
private:
    void newConnectionHandle(int sockfd,const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);
    
    using ConnectionMap=::unordered_map<std::string,std::shared_ptr<TcpConnection>>;//����ӳ���
    EventLoop* loop_;//���̵߳�EventLoop����

    const std::string name_; //����������
    const std::string ipPort_;//������������IP��ַ�Ͷ˿ں�
        
    std::unique_ptr<Acceptor> acceptor_;//��TcpServer�е���Ҫ��ɫ��ֻ�����ڼ���������
       std::unique_ptr<EventLoopThreadPool> threadPool_;
       
       ConnectionCallback connectionCallback_;//�û����õ����ӽ����ص�������acceptor��tcpconnection�������ʱ����
       MessageCallback messageCallback_;//�ж�д��Ϣʱ�Ļص�
       WriteCompleteCallback writeCompleteCallback_;//��Ϣд���ʱ�Ļص�
       
       ThreadInitCallback threadInitCallback_;//�̳߳س�ʼ��ʱ�Ļص�
        std::atomic_int started_;//�Ƿ�������
        int nextConnectionId_;//��һ�����ӵ�id
        ConnectionMap connections_;//����ӳ���
    };