#include"Buffer.h"
#include<sys/uio.h>
/*从fd上读取数据 Poller工作在LT模式
tcp数据是流式数据，读的过程中不知道最终数据的大小
*/

 ssize_t Buffer::readFd(int fd,int* saveError){
    char extrabuf[65536]={0};
    struct iovec vec[2];
    const size_t writeable = writableBytes();
    //先往writeable里写入数据
    vec[0].iov_base = begin();
    vec[0].iov_len = writeable;
    //不够再往extrabuf里写入数据
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const int iovcnt = (writeable<sizeof(extrabuf))?2:1;
    const ssize_t nread = ::readv(fd,vec,iovcnt);
    if(nread<0){
        *saveError = errno;
    }else if(nread<=writeable){//够写就不用扩容
        writeIndex_+=nread;
    }else{//writeable里已经写满了，extrabuf里也写入了数据
        writeIndex_=buffer_.size();//writeIndex_已经到达末尾
        append(extrabuf,nread-writeable);//将额外写进extrbuf里的数据扩容到buffer_里
    }
    return nread;
}
