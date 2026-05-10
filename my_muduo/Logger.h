#pragma once

#include<string>
//日志的级别
#include "noncopyable.h"
using namespace std;

//定义宏函数，方便用户使用日志功能。这个宏函数接受一个日志消息
#define LOG_INFO(Logmessage,...) \
    do{\
        Logger &logger=Logger::instance();\
        logger.setLogLevel(kInfo);\
        char buf[1024]={0};\
        snprintf(buf,1024,Logmessage,##__VA_ARGS__);\
        logger.func_Log(buf);  \
    } while(0)

#define LOG_WARN(Logmessage,...) \
    do{\
        Logger &logger=Logger::instance();\
        logger.setLogLevel(kWarn);\
        char buf[1024]={0};\
        snprintf(buf,1024,Logmessage,##__VA_ARGS__);\
        logger.func_Log(buf);  \
    } while(0)

#define LOG_ERROR(Logmessage,...) \
    do{\
        Logger &logger=Logger::instance();\
        logger.setLogLevel(kError);\
        char buf[1024]={0};\
        snprintf(buf,1024,Logmessage,##__VA_ARGS__);\
        logger.func_Log(buf);  \
    } while(0)

#define LOG_FATAL(Logmessage,...) \
    do{\
        Logger &logger=Logger::instance();\
        logger.setLogLevel(kFatal);\
        char buf[1024]={0};\
        snprintf(buf,1024,Logmessage,##__VA_ARGS__);\
        logger.func_Log(buf);  \
    } while(0)

//由于调试内容较多，所以通过条件编译来控制调试日志的输出。
// 当定义了MUDEBUG宏时，LOG_DEBUG宏才会输出调试日志；    
#ifdef  MUDEBUG  
#define LOG_DEBUG(Logmessage,...) \
    do{\
        Logger &logger=Logger::instance();\
        logger.setLogLevel(kDebug);\
        char buf[1024]={0};\
        snprintf(buf,1024,Logmessage,##__VA_ARGS__);\
        logger.func_Log(buf);  \
    } while(0)
#else
#define LOG_DEBUG(Logmessage,...)
#endif
//日志的宏定义结束    

enum LogLevel {
    kDebug , // 调试日志
    kInfo , // 普通信息日志
    kWarn , // 警告日志
    kError , // 错误日志
    kFatal , // 致命错误日志
};
//日志的级别结束

class Logger : noncopyable{
public:
    static Logger& instance(); // 获取日志实例函数
    void setLogLevel(int level); // 设置日志级别
    void func_Log(string msg);
private:
    int logLevel_; // 当前日志级别
    Logger(){}; // 构造，禁止外部创建实例 
    ~Logger(){}; // 析构，禁止外部销毁实例 
};
