#ifndef __HCServer_UTIL_H__
#define __HCServer_UTIL_H__

#include <unistd.h>
#include<pthread.h>
#include<string>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include<stdint.h>
using namespace std;
#include<vector>

namespace HCServer{
    
pid_t GetthreadId();//获取线程ID

u_int32_t GetFiberId();//获取协程ID

void Backtrace(vector<string> &bt,int size=64,int skip=1);//获取当前的调用栈
string BacktraceToString(int size=64,int skip=2,const string& prefix=" "); //获取当前栈信息的字符串

//时间ms
uint64_t GetcurrentMS();
uint64_t GetcurrentUS();


};



#endif // !__HCServer_UTIL_H__