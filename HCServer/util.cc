#include"util.h"
#include<execinfo.h>
#include<malloc.h>
#include<sys/time.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<dirent.h>
#include<string.h>
#include<stdlib.h>
#include<ifaddrs.h>

#include"log.h"
#include"fiber.h"


using namespace std;

namespace HCServer{

static HCServer::Logger::ptr g_logger=HCSERVER_LOG_NAME("system");

pid_t GetthreadId()
{
    return syscall(SYS_gettid);
}
u_int32_t GetFiberId()
{   
    return HCServer::Fiber::getFiberid();
}
//获取当前的调用栈的内容 bt:保存调用栈的容器   size：最多返回层数 skip：跳过栈顶的层数
void Backtrace(vector<string> &bt,int size,int skip)
{
    void ** array=(void **)malloc((sizeof (void*)*size));
    //backtrace()获取当前线程的调用堆栈，获取的信息将会被存放在array中，array是一个指针数组，
    //参数size用来指定array中可以保存多少个void*元素。
    //返回值是实际返回的void*元素个数。
    size_t s=::backtrace(array,size);
    //backtrace_symbols()将backtrace函数获取的信息转化为一个字符串数组，
    //参数array是backtrace()获取的堆栈指针
    //参数s是backtrace()返回值。
    char ** strings=backtrace_symbols(array,s);

    if(strings==NULL){
        HCSERVER_LOG_ERROR(g_logger)<<"backtrace_symbols error";
        return ;
    }

    for(size_t i=skip;i<s;++i)
    {
        bt.push_back(strings[i]);
    }
    free(strings);
    free(array);

}
//获取当前的调用栈的内容，并且转换成字符串形式
string BacktraceToString(int size,int skip,const string& prefix)
{
    vector<string> bt;
    Backtrace(bt,size,skip);
    stringstream ss;
    for(size_t i=0;i<bt.size();++i){
        ss<<prefix<<bt[i]<<endl;
    }
    return ss.str();
    
}

uint64_t GetcurrentMS()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec *1000 +tv.tv_usec /1000;
}
uint64_t GetcurrentUS() 
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec *1000 * 1000ul +tv.tv_usec;
}
}