#include"fiber.h"
#include"config.h"
#include"macro.h"
#include"log.h"
#include"scheduler.h"
#include<atomic>
using namespace std;

namespace HCServer{

static HCServer::Logger::ptr g_logger=HCSERVER_LOG_NAME("system");
static atomic<uint64_t> s_fiber_id{0};//协程id
static atomic<uint64_t> s_fiber_count{0};//协程的数量

static thread_local Fiber* t_fiber=nullptr;//当前协程指针
static thread_local Fiber::ptr t_threadFiber=nullptr;//主协程智能指针

static HCServer::ConfigVar<uint32_t>::ptr g_fiber_stack_size=HCServer::Config::Lookup<uint32_t>("fiber.stack_size",1024 * 1024 ,"fiber stack size");//栈大小

//栈内存分配器
class MallocStackAlloactor{
public:
    static void *Alloc(size_t size){
        return malloc(size);
    }
    static void DelAlloc(void *vp,size_t size){
        return free(vp);
    }
private:

};

using StackAllocator=MallocStackAlloactor;

uint64_t Fiber::getFiberid()
{
    if(t_fiber){
        return t_fiber->getId();
    }else{
        return 0;
    }
}

//初始化,并创建主协程
Fiber::Fiber()
{
    m_state=EXEC;
    setthis(this);
    //getcontext(&m_ctx)获取当前上下文信息（寄存器，计数器等信息），保存在参数m_ctx中
    if(getcontext(&m_ctx)){
        HCSERVER_ASSERT2(false,"getcontext");
    }
    ++s_fiber_count;
    HCSERVER_LOG_DEBUG(g_logger)<<"Fiber::Fiber()";
}
//初始化,并创建子协程
Fiber::Fiber(function<void()> cb,size_t stacksize, bool use_caller)
: m_id(++s_fiber_id),m_cb(cb)
{
    ++s_fiber_count;
    //如果stacksize不为0，则以stacksize的值为准，如果stacksize为0，则以配置文件中的stack_size为准
    m_stacksize=stacksize ? stacksize : g_fiber_stack_size->getValue();

    m_stack=StackAllocator::Alloc(m_stacksize);//创建协程运行的栈
    if(getcontext(&m_ctx))  //getcontext()获取当前上下文，并将当前的上下文保存到m_ctx(协程库)中
    {
        HCSERVER_ASSERT2(false,"getcontext");
    }

    //初始化协程运行的栈
    m_ctx.uc_link=nullptr;              //uc_link:上下文
    m_ctx.uc_stack.ss_sp=m_stack;       //uc_stack:该上下文中使用的栈，ss_sp对应的指针
    m_ctx.uc_stack.ss_size=m_stacksize; //uc_stack:该上下文中使用的栈，ss_size对应的大小

    makecontext(&m_ctx, &Fiber::MainFunc,0); //makecontext()创建一个新的上下文
    if(!use_caller) //使用协程调度器对协程进行调度
    {
        makecontext(&m_ctx,&Fiber::MainFunc,0); //makecontext() 创建一个新的上下文。
                                                //原理：修改通过getcontext()取得的上下文m_ctx(这意味着调用makecontext前必须先调用getcontext)。
                                                //      然后给该上下文指定一个栈空间m_ctx->uc.stack,设置后继的上下文m_ctx->uc_link.
    }else{    //不使用协程调度器对协程进行调度
        makecontext(&m_ctx,&Fiber::CallerMainFunc,0); //makecontext()创建一个新的上下文。
    }
    HCSERVER_LOG_DEBUG(g_logger)<<"Fiber::Fiber id="<<m_id;
}
Fiber::~Fiber()
{
    --s_fiber_count;
    if(m_stack){
        HCSERVER_ASSERT1(m_state==TERM||m_state==INIT||m_state==EXCEPT);
        StackAllocator::DelAlloc(m_stack,m_stacksize);//释放协程运行的栈
    }else{
        //判断是不是主协程，主协程没有回调函数并且状态为EXEC
        HCSERVER_ASSERT1(!m_cb);
        HCSERVER_ASSERT1(m_state==EXEC);
        Fiber* cur=t_fiber;
        if(cur==this){
            setthis(nullptr);
        }
    }
    HCSERVER_LOG_DEBUG(g_logger)<<"Fiber::~Fiber id="<<m_id;
}
//当协程执行完后，重置回调函数，并重置状态（init，tern）合理利用内存
void Fiber::reset(function<void()> cb)
{
    HCSERVER_ASSERT1(m_stack);
    HCSERVER_ASSERT1(m_state==TERM||m_state==INIT||m_state==EXCEPT);
    m_cb=cb;
    if(getcontext(&m_ctx))  //getcontext()获取当前上下文，并将当前的上下文保存到m_ctx(协程库)中
    {
        HCSERVER_ASSERT2(false,"getcontext");
    }

    //初始化协程运行的栈
    m_ctx.uc_link=nullptr;              //uc_link:上下文
    m_ctx.uc_stack.ss_sp=m_stack;       //uc_stack:该上下文中使用的栈，ss_sp对应的指针
    m_ctx.uc_stack.ss_size=m_stacksize; //uc_stack:该上下文中使用的栈，ss_size对应的大小

    makecontext(&m_ctx,&Fiber::MainFunc,0); //makecontext()创建一个新的上下文
    m_state=INIT;
}
//切换到当前的协程执行，拿到协程的执行权
void Fiber::swapIn()
{
    setthis(this);
    HCSERVER_ASSERT1(m_state!=EXEC);
    m_state=EXEC;
    //swapcontext(),调用了两个函数，第一次是调用了getcontext(t_threadFiber->m_ctx)然后再调用setcontext(m_ctx)
    if(swapcontext(&Scheduler::GetmainFiber()->m_ctx,&m_ctx)){
        HCSERVER_ASSERT2(false,"swapcontext");
    }
    // if(swapcontext(&t_threadFiber->m_ctx,&m_ctx)){
    //     HCSERVER_ASSERT2(false,"swapcontext");
    // }
}
//结束当前的协程,切换回主协程，归还协程的执行权(执行的是协程调度器的调度协程)
void Fiber::swapOut()
{
    setthis(Scheduler::GetmainFiber());
    if(swapcontext(&m_ctx,&Scheduler::GetmainFiber()->m_ctx)){
        HCSERVER_ASSERT2(false,"swapcontext");
    }
    // setthis(t_threadFiber.get());
    // if(swapcontext(&m_ctx,&t_threadFiber->m_ctx)){
    //     HCSERVER_ASSERT2(false,"swapcontext");
    // }
}  
//将当前协程强行置换成目标协程
void Fiber::call()
{
    setthis(this);
    m_state=EXEC;
    if(swapcontext(&t_threadFiber->m_ctx,&m_ctx)){
        HCSERVER_ASSERT2(false,"swapcontext");  
    }
}

//将当前线程切换到后台,并将主协程切换出来(执行的是当前线程的主协程)
void Fiber::back()
{
    setthis(t_threadFiber.get());
    if(swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
        HCSERVER_ASSERT2(false, "swapcontext");
    }
}

//设置当前正在运行的协程
void Fiber::setthis(Fiber* f)
{
    t_fiber=f;
}

//获取当前的协程，如果没有则创建一个新的主协程
Fiber::ptr Fiber::getthis()
{
    //当前存在协程，直接返回
    if(t_fiber){
        return t_fiber->shared_from_this();
    }
    //当前不存在协程，创建新的主协程
    Fiber::ptr main_fiber(new Fiber);
    HCSERVER_ASSERT1(t_fiber==main_fiber.get());    //判断当前协程是否是主协程
    t_threadFiber=main_fiber;
    return t_fiber->shared_from_this();


}

//协程让出执行权并且设置自己的状态为ready
void Fiber::YieldToReady()
{
    Fiber::ptr cur=getthis();
    cur->m_state=READY;
    cur->back();
}

//协程让出执行权并且设置自己的状态为hold
void Fiber::YieldToHold()
{
    Fiber::ptr cur=getthis();
    cur->m_state=HOLD;
    cur->back();
}

//统计当前的协程数量
uint64_t Fiber::TotalFibers()
{
    return s_fiber_count;
}

//协程执行的函数,执行完成返回到线程主协程
void Fiber::MainFunc()
{
    Fiber::ptr cur=getthis();   //共享指针+1
    HCSERVER_ASSERT1(cur);
    try //可能出现异常的代码
    {
        cur->m_cb();    //执行协程中的函数
        cur->m_cb=nullptr;
        cur->m_state=TERM;  //协程执行完
    }
    catch(exception& ex)   //异常处理
    {
        cur->m_state=EXCEPT;
        HCSERVER_LOG_ERROR(g_logger)<<"Fiber Except: "<<ex.what()
                <<" fiber_id="<<cur->getId()
                <<endl
                <<HCServer::BacktraceToString();
    }
    catch(...)  //异常处理
    {
        cur->m_state=EXCEPT;
        HCSERVER_LOG_ERROR(g_logger)<<"Fiber Except"
                <<" fiber_id="<<cur->getId()
                <<endl
                <<HCServer::BacktraceToString();
    }
        
    auto raw_ptr=cur.get(); //通过智能指针获取到裸指针
    cur.reset();    //释放cur(共享指针-1)
    raw_ptr->swapOut(); //切换回主协程

    HCSERVER_ASSERT2(false,"never reach fiber_id="+std::to_string(raw_ptr->getId()));
}

//协程执行的函数,执行完成返回到线程调度协程
void Fiber::CallerMainFunc()
{
    Fiber::ptr cur=getthis();   //共享指针+1
    HCSERVER_ASSERT1(cur);
    try //可能出现异常的代码
    {
        cur->m_cb();    //执行协程中的函数
        cur->m_cb=nullptr;
        cur->m_state=TERM;  //协程执行完
    }
    catch(exception& ex)   //异常处理
    {
        cur->m_state=EXCEPT;
        HCSERVER_LOG_ERROR(g_logger)<<"Fiber Except: "<<ex.what()
                <<" fiber_id="<<cur->getId()
                <<endl
                <<HCServer::BacktraceToString();
    }
    catch(...)  //异常处理
    {
        cur->m_state=EXCEPT;
        HCSERVER_LOG_ERROR(g_logger)<<"Fiber Except"
                <<" fiber_id="<<cur->getId()
                <<endl
                <<HCServer::BacktraceToString();
    }
        
    auto raw_ptr=cur.get(); //通过智能指针获取到裸指针
    cur.reset();    //释放cur(共享指针-1)
    raw_ptr->back(); //切换回主协程

    HCSERVER_ASSERT2(false,"never reach fiber_id="+std::to_string(raw_ptr->getId()));
}


}
