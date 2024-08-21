#ifndef _HCSERVER_SCHEDULER_H__
#define _HCSERVER_SCHEDULER_H__

#include<memory>
#include<vector>
#include<list>
#include"fiber.h"
//#include"mutex.h"
#include"thread.h"
using namespace std;

namespace HCServer{

//协程调度器类
//内部有一个线程池,支持协程在线程池里面切换
class Scheduler{
public:
    typedef shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    //threads 线程数量      use_caller 是否使用当前调用线程创建协程调度器     name 协程调度器名称
    Scheduler(size_t threads=1,bool use_caller=true,const string &name="");
    virtual ~Scheduler();

    const string & getname() const{return m_name;}//返回协程调度器名称
    static Scheduler* getthis();//获取当前的协程调度器
    static Fiber * GetmainFiber();//获取主协程

    void start(); //启动协程调度器
    void stop();  //停止协程调度器  

    //一个一个的调度协程
    //fc fiber或者funciton
    template<class FiberOrCb>
    void schedule(FiberOrCb fc,int threadid=-1){
        bool need_tickle=false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle=scheduleNoLock(fc,threadid);
        }
        if(need_tickle){
            tickle();
        }
    }

    //批量调度协程
    template<class InputIterator>
    void schedule(InputIterator begin,InputIterator end)
    {
        bool need_tickle=false;
        {
            MutexType::Lock lock(m_mutex);
            while (begin!=end)  //遍历协程数组
            {
                need_tickle=scheduleNoLock(&*begin,-1) || need_tickle; //查看协程调度器是否有任务
                ++begin;
            }
        }
        if(need_tickle){
            tickle();   //通知协程调度器有任务了
        }
    } 
protected:
    virtual void tickle();//通知协程调度器有任务了
    void run();//协程调度函数
    virtual bool stopping();//返回是否已经停止
    virtual void idle();//线程在没有协程任务做时会执行这个函数
    void setthis();//设置为当前的协程调度器

    //判断当前协程调度器里面有没有空闲的线程，有则返回true，没有则返回false
    bool hasIdleThreads(){ return m_idlethreadcount>0;}
    
private:
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc,int threadid)//无锁
    {
        bool need_tickle=m_fibers.empty();
        FiberAndThread ft(fc,threadid);
        if(ft.fiber||ft.cb){//协程是否存在
            m_fibers.push_back(ft);//往协程任务队列中添加ft任务
        }
        return need_tickle;
    }

private:
    //协程/函数/线程组
    struct FiberAndThread
    {
        Fiber::ptr fiber;
        function<void()> cb;
        int threadid;//协程对应的线程id

        FiberAndThread(Fiber::ptr f,int thr)
        :fiber(f),threadid(thr){}

        FiberAndThread(Fiber::ptr *f,int thr)
        :threadid(thr){
            fiber.swap(*f);
        }

        FiberAndThread(function<void()> f,int thr)
        :cb(f),threadid(thr){}

        FiberAndThread(function<void()> *f,int thr)
        :threadid(thr){
            cb.swap(*f);
        }

        FiberAndThread()
        :threadid(-1){}
        
        void reset(){
            fiber=nullptr;
            cb=nullptr;
            threadid=-1;
        }
    };
    

private:
    MutexType m_mutex;//互斥量
    vector<Thread::ptr> m_threads;//线程池
    list<FiberAndThread> m_fibers;//将要执行的协程队列:里面可以是function<void()>或者fiber
    Fiber::ptr m_rootFiber;//主协程
    string m_name;//协程调度器名称

protected:
    vector<int> m_threadIds;//线程id集合
    size_t m_threadcount=0;//线程数量
    atomic<size_t> m_activethreadcount={0};//活跃线程数量
    atomic<size_t> m_idlethreadcount={0};//空闲线程数量
    bool m_stopping=true;//是否已经停止：false：开启  true：关闭
    bool m_autostop=false;//是否自动停止
    int m_rootthreadId=0;//主线程id
};


}


#endif
