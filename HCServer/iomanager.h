#ifndef _HCSERVER_IOMANAGER_H__
#define _HCSERVER_IOMANAGER_H__


#include"scheduler.h"
#include"fiber.h"
#include"timer.h"
#include"HCServer.h"

namespace HCServer{

//IO管理类：负责 处理io事件、定时器的任务和协程的调度
class IOManager:public Scheduler,public TimerManager{
public:
    typedef shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;
    
    enum Event{
        NONE=0x0,
        READ=0x1,
        WRITE=0x4,
    };
private:

    //文件描述符（fd）上下文
    struct FdContext{
        typedef Mutex MutexType;
        //事件上下文
        struct EventContext{
            Scheduler* scheduler=nullptr;//事件执行的scheduler
            Fiber::ptr fiber;//事件协程
            function<void()> m_cb;//事件的回调函数
        };

        EventContext& getcontext(Event event);
        void resetContext(EventContext& ctx);
        void triggerEvent(Event event);

        int fd=0;//事件关联的句柄
        EventContext read;//读事件
        EventContext write;//写事件
        Event m_events=NONE;//已注册的事件
        MutexType m_mutex;//互斥量
    };

public:
    IOManager(size_t threads=1,bool use_caller=true,const string &name="");
    ~IOManager();
    
    //0:success  -1:error
    int addEvent(int fd,Event event,function<void()> cb=nullptr);//添加事件
    bool delEvent(int fd,Event event);//删除事件
    bool cancelEvent(int fd,Event event);//取消事件
    bool cancelAll(int fd);//取消所有事件
    
    static IOManager* Getthis();//获得当前的IOManger对象

protected:
    void tickle() override;//提醒线程有任务
    bool stopping()override;//返回是否已经停止
    void idle() override;//线程在没有协程任务做时会执行这个函数
    void onTimerInsertedAtFront() override;//当有新的定时器插入到存放定时器的容器的开始位置时,执行该函数

    void contextResize(size_t size);
    bool stopping(uint64_t& timeout);//判断是否可以停止
private:
    int m_epfd=0;//epoll的fd
    int m_tickleFds[2];//pipe管道读到的事件的fd
    atomic<size_t> m_pendingEventCount={0};//等待执行的事件数量
    RWMutex m_mutex;//读写互斥量
    vector<FdContext*> m_fdContexts;//文件描述符上下文的容器
};


}


#endif