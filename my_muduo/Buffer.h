#pragma once
#include<vector>
#include<string>
#include<algorithm>


//底层的缓冲区类，用于存储和操作网络数据
class Buffer{
public:
    static const size_t kCheapPrepend=8; // 预留8字节用于快速读取
    static const size_t kInitialSize=1024; // 初始缓冲区大小
    
       explicit Buffer(size_t initialSize=kInitialSize)
    :buffer_(kCheapPrepend+initialSize)
    ,readIndex_(kCheapPrepend)
    ,writeIndex_(kCheapPrepend)
    {}

    size_t readableBytes() const{ return writeIndex_-readIndex_;}
    size_t writableBytes() const{ return buffer_.size()-writeIndex_;}

    size_t prependableBytes() const{ return readIndex_-kCheapPrepend;}

    const char* peek()const{
        return begin()+readIndex_;//返回缓冲区中可读数据的起始地址
    }

//读取后调用的对缓冲区进行复位操作
    void retrieve(size_t len){
    if(len<readableBytes()){//只读一部分
        readIndex_+=len;
    }else{
        retrieveAll();
    }
    }

    void retrieveAll(){
    readIndex_=writeIndex_=kCheapPrepend;
    }

//onMessage中上报的Buffer数据转为string类型
    std::string triveAllAsString(){
        return retriveAsString(readableBytes());
    } 

    std::string retriveAsString(size_t len){
    std::string result(peek(),len);
    retrieve(len);//上面把缓冲区可读的数据已经读取出来，这里进行复位操作
    return result;
    }

    void ensureWriteableBytes(size_t len){
        if(len>writableBytes()){  // 如果可写空间不足，需要扩展缓冲区
            expandSpace(len);//扩容函数
        }
    }

    //将发送data中的len个字节添加到到缓冲区的可写空间中
    void append(const char*data,size_t len){
        ensureWriteableBytes(len);//先确保缓冲区有足够空间
        std::copy(data,data+len,beginWrite());
        writeIndex_+=len;
    }

    char* beginWrite(){
        return begin()+writeIndex_;
    }

    const char* beginWrite() const{
        return begin()+writeIndex_;   
    }
    
    ssize_t readFd(int fd,int* saveError);//从文件描述符fd读取数据到缓冲区中

    ssize_t writeFd(int fd,int* saveError);//将缓冲区中的数据写入文件描述符fd
private:
    char* begin(){
        return &*buffer_.begin();//
    }
    const char* begin() const{
        return &*buffer_.begin();
    }

    void expandSpace(size_t len){
//如果已经读了n个，readIndex_+=n，实际可写空间还应该算上前面读走后空出来的，这里的扩容
    if(writableBytes()+prependableBytes()<len+kCheapPrepend){
        buffer_.resize(writeIndex_+len);
    }else{//将可读数据复制到缓冲区的开头，把前面的空闲空间与后面连起来
       size_t readbleBytes=readableBytes();
        std::copy(begin()+readIndex_,begin()+writeIndex_,begin()+kCheapPrepend);
        readIndex_=0+kCheapPrepend;
        writeIndex_=kCheapPrepend+readbleBytes;
    }
    }

    std::vector<char> buffer_;
    size_t readIndex_;
    size_t writeIndex_;
};

/*
buffer_.begin()	返回 vector 的迭代器，指向第一个元素
*buffer_.begin()	解引用迭代器，得到第一个元素的引用（char&）
&*buffer_.begin()	对引用取地址，得到指向第一个元素的指针（char*）
*/