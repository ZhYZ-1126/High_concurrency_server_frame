/*
*线程模块
*创建当前线程的子线程时还会创建他的回调函数
*/


#ifndef _HCSERVER_THREAD_H__
#define _HCSERVER_THREAD_H__

#include<thread>
#include<functional>
#include<memory>
#include<pthread.h>
#include<stdint.h>
#include<atomic>
#include"noncopyable.h"
#include"mutex.h"

using namespace std; 

namespace HCServer{

//线程类
class Thread{
public: 
    typedef shared_ptr<Thread> ptr;
    Thread(function<void()> cb,const string & name);
    ~Thread();

    pid_t Getid() const{return m_id;}
    const string& getname() const {return m_name;}

    void join();
    static Thread *GetThis();//获取当前线程，提供给那些需要获取当前线程的系统使用
    static const string& Getname();//获取当然线程的名称，该方法供日志使用
    static void SetName(const string& name);
private:
    //禁止默认拷贝
    Thread(const Thread&)=delete;
    Thread(const Thread&&)=delete;
    Thread& operator=(const Thread&)=delete;

    static void * run(void *arg);//线程执行函数
private:
    pid_t m_id=-1;//线程id
    pthread_t m_thread=0;//线程标识符
    function<void()> m_cb;//回调
    string m_name;//线程名称
    Semaphore m_semaphore;//信号量
};


}


#endif