#include "Timestamp.h"
#include <ctime>
#include <time.h>
#include <chrono>


Timestamp::Timestamp():microSeconds_(0){};

Timestamp::Timestamp(int64_t microSeconds):microSeconds_(microSeconds)
{};

Timestamp Timestamp::now() {
    auto now= std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    // 转换为微秒
    int64_t microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    return Timestamp(microseconds);
}

string Timestamp::toString() const {
   char buffer[128]={0};
   // 将微秒转换为秒
   time_t seconds = microSeconds_ / 1000000;
   tm *tm_time = localtime(&seconds);
   snprintf(buffer, 128, "%4d-%02d-%02d %02d:%02d:%02d",
         tm_time->tm_year + 1900, tm_time->tm_mon + 1, tm_time->tm_mday,
         tm_time->tm_hour, tm_time->tm_min, tm_time->tm_sec);
    return string(buffer);
}

/*测试运行代码
#include <iostream>
 int main(){
    Timestamp ts=Timestamp::now();
    std::cout<<ts.toString()<<std::endl;
    return 0;
}*/