#include"scheduler.h"
#include<iostream>
#include"log.h"
#include"fiber.h"
#include"hook.h"
#include"macro.h"
using namespace std;

namespace HCServer{

static HCServer::Logger::ptr g_logger=HCSERVER_LOG_NAME("system");

static thread_local Scheduler* t_scheduler=nullptr;
static thread_local Fiber* t_fiber=nullptr;//主协程

//这个地方的关键点在于，是否把创建协程调度器的线程放到协程调度器管理的线程池中。
//如果不放入，那这个线程专职协程调度；如果放的话，那就要把协程调度器封装到一个协程中，称之为主协程或协程调度器协程。
Scheduler::Scheduler(size_t threads,bool use_caller,const string &name)
:m_name(name)
{
    HCSERVER_ASSERT1(threads>0);
    if(use_caller){
        HCServer::Fiber::getthis();//得到主协程指针，如果没有则初始化一个主协程
        --threads;

        HCSERVER_ASSERT1(getthis()==nullptr);//当前的线程已经有一个协程调度器了，不能再拥有其他的协程调度器
        t_scheduler=this;
        m_rootFiber.reset(new Fiber(bind(&Scheduler::run,this),0,true));//主协程存协程调度器的执行函数
        HCServer::Thread::SetName(m_name);
        t_fiber=m_rootFiber.get();

        m_rootthreadId=HCServer::GetthreadId();
        m_threadIds.push_back(m_rootthreadId);

    }else{
        m_rootthreadId=-1;
    }
    m_threadcount=threads;
}
Scheduler::~Scheduler()
{
    HCSERVER_ASSERT1(m_stopping);
    if(getthis()==this){
        t_scheduler=nullptr;
    }
}
//获取当前的协程调度器
Scheduler* Scheduler::getthis()
{   
    return t_scheduler;
}
//获取主协程
Fiber * Scheduler::GetmainFiber()
{
    return t_fiber;
}
//启动协程调度器
void Scheduler::start()
{
    MutexType::Lock lock(m_mutex);
    if(!m_stopping){//协程调度器已开启
        return ;
    }
    m_stopping=false;

    HCSERVER_ASSERT1(m_threads.empty());

    m_threads.resize(m_threadcount);
    //线程池创建线程
    for(size_t i=0;i<m_threadcount;++i)
    {
        //将可调用对象(函数)和参数一起绑定，绑定后的结果使用function<void()>进行保存
        //顺便创建线程执行协程中run执行函数
        m_threads[i].reset(new Thread(bind(&Scheduler::run,this),m_name+"_"+to_string(i)));    
        m_threadIds.push_back(m_threads[i]->Getid());
    }
    lock.unlock(); //没有则死锁了,所有需要解锁
}    
//停止协程调度器
void Scheduler::stop()
{
    //关闭那些通过当前线程来创建协程调度器的线程，并且只有一个线程的情况
    m_autostop=true;
    //判断是否符合停止协程调度器的条件
    //创建协程调度器的线程执行的run函数中的主协程的状态为初始化或者结束，说明该协程根本没跑起来或者已经结束了
    if(m_rootFiber&& m_threadcount==0&& (m_rootFiber->getState()==Fiber::TERM|| m_rootFiber->getState()==Fiber::INIT))  
    {
        HCSERVER_LOG_INFO(g_logger)<<this<<" stopped";
        m_stopping=true;

        if(stopping())
        {
            return;
        }
    }

    //关闭那些含有多个线程的情况，先对每个线程进行唤醒，最后再对主线程进行唤醒
    if(m_rootthreadId!=-1)
    {
        HCSERVER_ASSERT1(getthis()==this);
    }else{
        HCSERVER_ASSERT1(getthis()!=this);
    }

    m_stopping=true;//关闭调度器
    for(size_t i=0;i<m_threadcount;++i)
    {
        tickle();   //通知协程调度器有任务(唤醒)
    }

    if(m_rootFiber)
    {
        tickle();   //通知协程调度器有任务(唤醒)
    }

    if(m_rootFiber)
    {   
        if(!stopping())
        { 
            m_rootFiber->call();
        }
    }

    vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);   //交换vector元素
    }

    //挂起当前线程，等待i线程执行完后才能执行当前线程
    for(auto& i:thrs)
    {
        i->join();  //等待线程执行完成
    }

}

void Scheduler::setthis()
{
    t_scheduler=this;
}
//协程调度函数,协调协程和线程的关系，当线程没有任务做时会做些其他的事情
void Scheduler::run()
{
    HCSERVER_LOG_INFO(g_logger)<<"Scheduler run";
    setthis();//将调度器的指针置为自己
    set_hook_enable(true);
    if(HCServer::GetthreadId()!=m_rootthreadId){//当前的线程id不等于主线程id
        t_fiber=Fiber::getthis().get();
    }

    //当调度的任务做完后会执行idle协程
    Fiber::ptr idle_fiber(new Fiber(bind(&Scheduler::idle,this))); 
    
    Fiber::ptr cb_fiber;
    FiberAndThread ft;

    while(true){
        //线程在协程任务队列中拿到对应的协程任务
        ft.reset();
        bool tickle_me=false; //判断是否要通知协程调度器有任务
        bool is_active=false;
        {
            MutexType::Lock lock(m_mutex);
            auto it=m_fibers.begin();
            while(it!=m_fibers.end()){
                //threadid是指当前协程任务的线程id，当当前协程任务的线程id存在并且该id不等于当前线程的线程id，
                //说明现在的协程任务不是在当前的线程中执行的，则需要通知一下其他线程执行该协程任务，并且++it跳过当前协程任务
                if(it->threadid!=-1&&it->threadid!=HCServer::GetthreadId()){
                    ++it;
                    tickle_me=true;
                    continue;
                }
                HCSERVER_ASSERT1(it->fiber||it->cb);
                //协程处于执行状态
                if(it->fiber&&it->fiber->getState()==Fiber::EXEC){
                    ++it;
                    continue;
                }
                ft=*it;//拿到当前协程任务
                m_fibers.erase(it);
                ++m_activethreadcount;//活跃线程数+1
                is_active=true;
                break;
            }
        }
        if(tickle_me){
            tickle();//唤醒其他线程
        }


        //执行该协程队列中的任务
        if(ft.fiber&&(ft.fiber->getState()!=Fiber::TERM&&ft.fiber->getState()!=Fiber::EXCEPT)){//Fiber
            ft.fiber->swapIn();//执行子协程
            --m_activethreadcount;//活跃线程数-1

            if(ft.fiber->getState()==Fiber::READY){
                schedule(ft.fiber);//将该协程继续放进协程任务队列中
            }else if(ft.fiber->getState()!=Fiber::TERM&&ft.fiber->getState()!=Fiber::EXCEPT){
                ft.fiber->m_state=Fiber::HOLD;
            }
            ft.reset();
        }else if(ft.cb){//function<void()> 回调函数
            if(cb_fiber){
                cb_fiber->reset(ft.cb);//重置回调函数
            }else{
                cb_fiber.reset(new Fiber(ft.cb));
            }
            ft.reset();

            cb_fiber->swapIn();//执行子协程
            --m_activethreadcount;//活跃线程数-1

            if(cb_fiber->getState()==Fiber::READY){
                schedule(cb_fiber);//将该协程继续放进协程任务队列中
                cb_fiber.reset();
            }else if(cb_fiber->getState()==Fiber::TERM||cb_fiber->getState()==Fiber::EXCEPT){
                cb_fiber->reset(nullptr);
            }else{
                cb_fiber->m_state=Fiber::HOLD;
                cb_fiber.reset();  
            }
        }else{//当前线程没有协程任务执行，执行idle函数，
            if(is_active){
                --m_activethreadcount;
                continue;
            }
            if(idle_fiber->getState()==Fiber::TERM){
                HCSERVER_LOG_INFO(g_logger)<<"idle fiber state is TERM";
                break; 
            }
            
            ++m_idlethreadcount;//空闲线程数+1
            idle_fiber->swapIn();
            --m_idlethreadcount;//空闲线程数-1

            

            if(idle_fiber->getState()!=Fiber::TERM&&idle_fiber->getState()!=Fiber::EXCEPT){
                idle_fiber->m_state=Fiber::HOLD;
            }
        }
    }
}

void Scheduler::tickle()
{
    HCSERVER_LOG_INFO(g_logger)<<"tickle";
}   
//返回是否已经停止
bool Scheduler::stopping()
{   
    MutexType::Lock lock(m_mutex);
    return m_autostop&&m_stopping&&m_fibers.empty()&&m_activethreadcount==0;
}
void Scheduler::idle()
{
    HCSERVER_LOG_INFO(g_logger)<<"Scheduler idle";
    while(!stopping()){
        HCServer::Fiber::YieldToHold();
    }
}


}
