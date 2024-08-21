#ifndef __HCServer_FIBER_H__
#define __HCServer_FIBER_H__

#include<ucontext.h>
#include<iostream>
#include<memory>
#include"thread.h"
using namespace std;

namespace HCServer
{
class Scheduler;
class Fiber:public enable_shared_from_this<Fiber>
{
friend class Scheduler;
public:
    typedef shared_ptr<Fiber> ptr;

    enum State{
        INIT,   //初始化状态(刚开始)
        EXEC,   //执行中状态(创建时)
        READY,  //可执行状态(切换后)
        HOLD,   //暂停状态(切换后)
        EXCEPT,  //异常状态(异常时)
        TERM   //结束状态(结束时)
    };

private:
    Fiber();//禁止默认构造

public:
    Fiber(function<void()> cb,size_t stacksize=0, bool use_caller=false);
    ~Fiber();

    void reset(function<void()> cb);//当协程执行完后，重置回调函数，并重置状态（init，term）合理利用内存
    void swapIn();//切换到当前的协程执行，拿到协程的执行权
    void swapOut();//结束当前的协程，归还协程的执行权
    void call();//唤醒
    void back();//切回主协程

    uint64_t getId() const {return m_id;}  //返回协程id
    State getState() const {return m_state;}//返回协程状态

public:
    static void setthis(Fiber* f);//设置当前协程
    static Fiber::ptr getthis();//获取当前的协程
    static void  YieldToReady();//协程让出执行权并且设置自己的状态为ready
    static void  YieldToHold();//协程让出执行权并且设置自己的状态为hold
    static uint64_t TotalFibers();//统计当前的协程数量
    static void MainFunc();//协程执行的函数 ,执行完成返回到线程主协程
    static void CallerMainFunc();//协程执行的函数,执行完成返回到线程调度协程
    static uint64_t getFiberid();//获取协程id

private:
    uint64_t m_id=0;//协程id
    void * m_stack=nullptr;//栈
    uint32_t m_stacksize=0;//协程栈的大小
    State m_state=INIT;//协程状态
    ucontext_t m_ctx;//协程对象


    function<void()> m_cb;
};


}


#endif