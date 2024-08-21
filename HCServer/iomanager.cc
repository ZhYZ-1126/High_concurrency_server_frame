#include"iomanager.h"
#include"log.h"
// #include"util.h"
// #include"macro.h"
#include<errno.h>
#include<unistd.h>
#include<fcntl.h>
#include<string.h>
#include<vector>
#include<sys/epoll.h>
#include<iostream>
using namespace std;

namespace HCServer{

static HCServer::Logger::ptr g_logger=HCSERVER_LOG_NAME("system");

//返回事件上下文
IOManager::FdContext::EventContext& IOManager::FdContext::getcontext(IOManager::Event event)
{
    switch(event){
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            HCSERVER_ASSERT2(false,"getcontext");
    }
    throw std::invalid_argument("getContext invalid event");
}
//重置上下文
void IOManager::FdContext::resetContext(EventContext& ctx)
{
    ctx.scheduler=nullptr;
    ctx.fiber.reset();
    ctx.m_cb=nullptr;
}
//插入协程任务队列
void IOManager::FdContext::triggerEvent(IOManager::Event event)
{
    HCSERVER_ASSERT1(m_events & event);
    
    m_events=(Event)(m_events & ~event);
    EventContext &ctx=getcontext(event);
    if(ctx.m_cb){
        ctx.scheduler->schedule(&ctx.m_cb);
    }else{
        ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler=nullptr;
    return ;
}

IOManager::IOManager(size_t threads,bool use_caller,const string &name)
:Scheduler(threads,use_caller,name)
{   
    HCSERVER_LOG_INFO(g_logger)<<"IOManager";
    ////创建epoll模型，m_epfd指向红黑树的根节点
    m_epfd=epoll_create(5000);
    HCSERVER_ASSERT1(m_epfd>0);
    //得到目标文件的fd，m_tickleFds[0]为读事件的fd，m_tickleFds[1]为写事件的fd
    int rt=pipe(m_tickleFds);
    HCSERVER_ASSERT1(!rt);//rt是读事件

    epoll_event event;
    memset(&event,0,sizeof(epoll_event));
    event.events=EPOLLIN | EPOLLET;
    event.data.fd=m_tickleFds[0];

    rt=fcntl(m_tickleFds[0],F_SETFL,O_NONBLOCK);//设置为非阻塞
    HCSERVER_ASSERT1(!rt);

    //添加对读事件的监听，通过m_epfd找到该fd，m_tickleFds[0]为读事件的fd
    rt=epoll_ctl(m_epfd,EPOLL_CTL_ADD,m_tickleFds[0],&event);
    HCSERVER_ASSERT1(!rt);    

    contextResize(32);

    start();//启动调度器，创建线程池
}
IOManager::~IOManager()
{
    HCSERVER_LOG_INFO(g_logger)<<"~IOManager";
    stop();//停止调度器，执行协程队列任务中的协程任务
    //关闭epoll红黑树根节点、读fd、写fd
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for(size_t i=0;i<m_fdContexts.size();++i){
        if(m_fdContexts[i]){
            delete m_fdContexts[i];
        }
    }
}
//重置文件描述符上下文容器
void IOManager::contextResize(size_t size)
{
    m_fdContexts.resize(size);
    for(size_t i=0;i<m_fdContexts.size();++i){
        if(!m_fdContexts[i]){
            m_fdContexts[i]=new FdContext;
            m_fdContexts[i]->fd=i;
        }
    }
}
//往epoll红黑树里面添加 读/写 事件
int IOManager::addEvent(int fd,Event event,function<void()> cb)
{
    FdContext * fd_ctx=nullptr;
    RWMutexType::Readlock lock(m_mutex);
    if((int)m_fdContexts.size() > fd){//容器的大小放得下该句柄
        fd_ctx=m_fdContexts[fd];
        lock.unlock();
    }else{//容器的大小放不下该句柄
        lock.unlock();
        RWMutexType::Writelock lock2(m_mutex);
        contextResize(fd*1.5);
        fd_ctx=m_fdContexts[fd];
    }

    FdContext::MutexType::Lock lock2(fd_ctx->m_mutex);
    if(fd_ctx->m_events & event){//之前已经添加过某种事件了，如果再次添加一样的事件会报错
        HCSERVER_LOG_ERROR(g_logger)<<"addEvent assert fd ="<<fd<<" event="<<event
                                    <<" fd_ctx.event="<<fd_ctx->m_events;
        HCSERVER_ASSERT1(!(fd_ctx->m_events & event));                            
    }
    
    //如果当前的fd已经定义了事件，则当前对epoll红黑树的所做的操作为修改，反之为添加
    int op=fd_ctx->m_events? EPOLL_CTL_MOD:EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events=EPOLLET | fd_ctx->m_events | event;//事件类型
    epevent.data.ptr=fd_ctx;

    //添加或者修改 读/写 事件
    int rt=epoll_ctl(m_epfd,op,fd,&epevent);
    if(rt){
        HCSERVER_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd<<","<<op<<","<<fd<<","<<epevent.events
                                    <<"):"<<rt<<"("<<"errno"<<") ("<<strerror(errno)<<")";
        return -1;
    }

    ++m_pendingEventCount;

    fd_ctx->m_events=(Event)(fd_ctx->m_events | event);
    FdContext::EventContext& event_ctx=fd_ctx->getcontext(event);//拿到当前 读/写 事件的指针
    HCSERVER_ASSERT1(!event_ctx.scheduler&&!event_ctx.fiber&&!event_ctx.m_cb);

    //初始化该 读/写 事件的调度器、回调函数/协程
    event_ctx.scheduler=Scheduler::getthis();
    if(cb){
        event_ctx.m_cb.swap(cb);
    }else{
        event_ctx.fiber=Fiber::getthis();
        HCSERVER_ASSERT2(event_ctx.fiber->getState()==Fiber::EXEC,"state=" << event_ctx.fiber->getState());
    }
    return 0;
}
//删除事件
bool IOManager::delEvent(int fd,Event event)
{
    RWMutexType::Readlock lock(m_mutex);
    if((int)m_fdContexts.size()<=fd){//fd>容器大小
        return false;
    }
    FdContext* fd_ctx=m_fdContexts[fd];//拿到该fd的上下文信息
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->m_mutex);

    if(!(fd_ctx->m_events & event)){
        return false;
    }

    Event new_events=(Event)(fd_ctx->m_events & ~event);
    int op=new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;//无事件是修改，有事件是删除
    epoll_event epevent;
    epevent.events=EPOLLET | new_events;    //events事件是int类型
    epevent.data.ptr=fd_ctx;    //委托内核监控的文件描述符————events存储用户数据（ptr）

    int rt=epoll_ctl(m_epfd,op,fd,&epevent);//删除或者修改 读/写 事件
    if(rt){
        HCSERVER_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd<<","<<op<<","<<fd<<","<<epevent.events
                                    <<"):"<<rt<<"("<<"errno"<<") ("<<strerror(errno)<<")";
        return false;
    }

    --m_pendingEventCount;

    fd_ctx->m_events=new_events;
    FdContext::EventContext& event_ctx=fd_ctx->getcontext(event);//拿到当前  读/写 事件的指针
    fd_ctx->resetContext(event_ctx);
    return true;

}
//取消事件（取消事件不等于删除事件，取消事件会将该事件的读/写事件添加到执行队列中，并且执行）
bool IOManager::cancelEvent(int fd,Event event)
{
    RWMutexType::Readlock lock(m_mutex);
    if((int)m_fdContexts.size()<=fd){
        return false;
    }
    FdContext* fd_ctx=m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->m_mutex);

    if(!(fd_ctx->m_events & event)){
        return false;
    }
    Event new_events=(Event)(fd_ctx->m_events & ~event);
    int op=new_events? EPOLL_CTL_MOD : EPOLL_CTL_DEL;   
    epoll_event epevent;
    epevent.events=EPOLLET | new_events;    //events事件是int类型
    epevent.data.ptr=fd_ctx;    //委托内核监控的文件描述符————events存储用户数据（ptr）

    int rt=epoll_ctl(m_epfd,op,fd,&epevent);//删除或者修改 读/写 事件
    if(rt){
        HCSERVER_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd<<","<<op<<","<<fd<<","<<epevent.events
                                    <<"):"<<rt<<"("<<"errno"<<") ("<<strerror(errno)<<")";
        return false;
    }

    fd_ctx->triggerEvent(event);//触发该fd的 读/写 事件
    --m_pendingEventCount;

    return true;
}
//取消所有事件
bool IOManager::cancelAll(int fd)
{
    RWMutexType::Readlock lock(m_mutex);
    if((int)m_fdContexts.size()<=fd){
        return false;
    }
    FdContext* fd_ctx=m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->m_mutex);
    if(!fd_ctx->m_events){
        return false;
    }

    int op=EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events=0;    
    epevent.data.ptr=fd_ctx;    

    int rt=epoll_ctl(m_epfd,op,fd,&epevent);//删除或者修改 读/写 事件
    if(rt){
        HCSERVER_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd<<","<<op<<","<<fd<<","<<epevent.events
                                    <<"):"<<rt<<"("<<"errno"<<") ("<<strerror(errno)<<")";
        return false;
    }
    
    if(fd_ctx->m_events & READ)
    {
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }else if(fd_ctx->m_events & WRITE){
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }

    HCSERVER_ASSERT1(fd_ctx->m_events==0);
    return true;
}
//获得当前的IOManger对象
IOManager* IOManager::Getthis()
{
    return dynamic_cast<IOManager*>(Scheduler::getthis());
}
//通知协程调度器有任务,唤醒线程
void IOManager::tickle() 
{   
    if(!hasIdleThreads()){//当前没有空闲的线程
        return ;
    }
    int rt=write(m_tickleFds[1],"T",1);//m_tickleFds[1]:写事件的fd
    HCSERVER_ASSERT1(rt==1);
}
//判断是否可以停止
bool IOManager::stopping(uint64_t& timeout)    
{
    timeout=getNextTimer();
    return timeout==~0ull && m_pendingEventCount==0 && Scheduler::stopping();
}
//满足条件才可以停止协程调度器
bool IOManager::stopping()
{
    uint64_t timeout=0;
    return stopping(timeout);
}
//检测epoll树是否有就绪(触发事件)的节点,且执行节点对应的函数
//当协程调度器里面没有任务可以做时会调用到idle函数，在这个函数中，通过epoll_wait函数阻塞监听有没有事件可以被唤醒，
//唤醒后处理这些事件，处理完后会交出协程的执行权给协程调度器（回到run函数中），
void IOManager::idle()
{
    HCSERVER_LOG_DEBUG(g_logger) << "IOManager idle";
    epoll_event* events=new epoll_event[256]();
    shared_ptr<epoll_event> shared_events(events,[](epoll_event* ptr){
        delete[] ptr;
    });

    while(true){
        uint64_t next_timeout=0;    //存储定时器的执行时间
        if(stopping(next_timeout)){
            HCSERVER_LOG_INFO(g_logger)<<"name="<<getname()<<" idle stopping exit";
            break;
        }
        int rt=0;
        do{
            static const int MAX_TIMEOUT=3000;//最大的超时时间
            if(next_timeout!=~0ull)
            {
                next_timeout=(int)next_timeout>MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
            }else{
                next_timeout=MAX_TIMEOUT;
            }
            rt=epoll_wait(m_epfd,events,256,(int)next_timeout);//epoll_wait()阻塞监听有没有事件触发，rt为满足条件的总个数
            if(rt<0 && errno==EINTR){//epoll_wait()出错
                
            }else{
                break;
            }
        }while(true);

        vector<function<void()>> cbs;
        listExpiredCb(cbs);//拿到定时器的执行函数
        if(!cbs.empty())    
        {
            schedule(cbs.begin(),cbs.end());//将定时器中的执行函数插入协程任务队列中
            cbs.clear();
        }

        //将epoll_wait的返回值：监听到的事件数，作为监听上限
        for(int i=0;i<rt;i++){
            epoll_event & event=events[i];//event：监听到的事件
            if(event.data.fd==m_tickleFds[0]){//判断是不是读事件
                uint8_t dummy[256];
                while(read(m_tickleFds[0], dummy, sizeof(dummy)) > 0);
                continue;
            }

            FdContext* fd_ctx=(FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->m_mutex);
            if(event.events & (EPOLLERR |EPOLLHUP)){
                event.events |=EPOLLIN|EPOLLOUT & fd_ctx->m_events;
            }
            int real_events=NONE;
            if(event.events & EPOLLIN){//监听事件为读
                real_events |= READ;
            }
            if(event.events & EPOLLOUT){//监听事件为写
                real_events |= WRITE;
            }

            if((fd_ctx->m_events & real_events)==NONE){//无事件
                continue;
            }

            int left_events=(fd_ctx->m_events & ~real_events);
            int op=left_events? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events=EPOLLET|left_events;

            int rt2=epoll_ctl(m_epfd,op,fd_ctx->fd, &event);
            if(rt2)
            {
                HCSERVER_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd<<","<<op<<","<<fd_ctx->fd<<","<<event.events
                                    <<"):"<<rt2<<"("<<"errno"<<") ("<<strerror(errno)<<")";
                continue;
            }

            if(real_events & READ){
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if(real_events & WRITE){
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }
        Fiber::ptr cur=Fiber::getthis();
        auto raw_ptr=cur.get();
        cur.reset();
        raw_ptr->swapOut();
    }
}

//当有新的定时器插入到存放定时器的容器的开始位置时,执行该函数
void IOManager::onTimerInsertedAtFront()
{
    tickle();
}

}
