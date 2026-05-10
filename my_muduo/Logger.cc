#include "Logger.h"
#include "Timestamp.h"

#include<iostream>
Logger & Logger::instance() {
    static Logger logger; // 通过用局部静态变量实现单例模式
    return logger;
}
//通过调用此函数，程序可以在任何地方获取到同一个Logger对象，确保整个应用中只有一个日志记录器实例。

void Logger::setLogLevel(int level) {
    logLevel_ = (LogLevel)level; // 将整数转换为LogLevel枚举类型
}

void Logger::func_Log(string msg) {
    // 根据当前日志级别输出日志信息
   switch (logLevel_) {
        case kDebug:
            cout << "[DEBUG] " << endl;
            break;
        case kInfo:
            cout << "[INFO] "  << endl;
            break;
        case kWarn:
            cout << "[WARN] "  << endl;
            break;
        case kError:
            cout << "[ERROR] "  << endl;
            break;
        case kFatal:
            cout << "[FATAL] "  << endl ;
            break;
        default:
            cout << "[UNKNOWN] " << endl;
    }
    //输出日志信息和时间戳
    cout  <<Timestamp::now().toString() <<" : "<<msg << endl;
}
    
