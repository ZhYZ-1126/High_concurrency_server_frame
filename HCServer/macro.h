#ifndef __HCSERVER_MACRO_H
#define __HCSERVER_MACRO_H

#include<string.h>
#include<assert.h>
#include"util.h"

//成功概率大，小，使用可以提高性能
#if defined __GNUC__ || defined __llvm__    //有些编译器执行，有些不执行
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立(成功几率大)
#   define HCSERVER_LIKELY(x)       __builtin_expect(!!(x), 1)
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立(成功几率小)
#   define HCSERVER_UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#   define HCSERVER_LIKELY(x)      (x)
#   define HCSERVER_UNLIKELY(x)      (x)
#endif



//assert():用于在运行时检查一个条件是否为真，如果条件不满足，则运行时将终止程序的执行并输出一条错误信息。
#define HCSERVER_ASSERT1(x)\
    if(HCSERVER_UNLIKELY(!(x))){\
        HCSERVER_LOG_ERROR(HCSERVER_LOG_ROOT())<<"ASSERTION: " #x \
                    <<"\nbacktrace:\n"\
                    <<HCServer::BacktraceToString(100,2,"    ");\
        assert(x);\
    }

#define HCSERVER_ASSERT2(x,w)\
    if(HCSERVER_UNLIKELY(!(x))){\
        HCSERVER_LOG_ERROR(HCSERVER_LOG_ROOT())<<"ASSERTION: " #x \
                    <<"\n"<<w\
                    <<"\nbacktrace:\n"\
                    <<HCServer::BacktraceToString(100,2,"    ");\
        assert(x);\
    }


#endif