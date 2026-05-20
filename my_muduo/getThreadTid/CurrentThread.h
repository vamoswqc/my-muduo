#pragma once
#include <unistd.h>
#include <sys/syscall.h>


namespace CurrentThread
{
    extern __thread int t_cachedTid;
    void cacheTid();

    inline int tid(){
   //第一次调用，会调用cacheTid()函数获取线程ID，后续调用直接返回缓存的线程ID
    if(__builtin_expect(t_cachedTid==0,0)){
        cacheTid();
    }
    return t_cachedTid;
   }   
}