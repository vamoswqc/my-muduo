#pragma once
#include <cstdint>
#include <string>
using namespace std;

class Timestamp{
    public:
        Timestamp();
        explicit Timestamp(int64_t microSeconds);  
        static Timestamp now();  //获取当前时间戳
        string toString() const;  //将时间戳转换为字符串,并且只有const对象才能调用
    private:
        int64_t microSeconds_;
};
/*在 C++ 中，explicit 关键字用于修饰单参数构造函数，其核心作用是禁止隐式类型转换。
防止编译器自动将 int64_t 类型的值隐式转换为 Timestamp 对象。
Timestamp ts = 123456789; // 若不加这里123456789 会隐式转换为 Timestamp 对象
Timestamp ts(123456789);   // 显式调用构造函数，明确表示创建一个 Timestamp 对象而不是赋值
一般建议单参数构造函数使用 explicit 关键字，以避免隐式类型转换。
*/