#include"hook.h"
#include"log.h"
#include"config.h"
#include"fiber.h"
#include"fd_manager.h"
#include"iomanager.h"
#include<dlfcn.h>

using namespace std;


static HCServer::Logger::ptr g_logger = HCSERVER_LOG_NAME("system");

namespace HCServer{


static thread_local bool t_hook_enable=false;//是否使用hook

//tcp连接超时时间
static HCServer::ConfigVar<int>::ptr g_tcp_connect_timeout=HCServer::Config::Lookup("tcp.connect.timeout",5000,"tcp connect timeout");

#define HOOK_FUN(XX) \
    XX(sleep)   \
    XX(usleep)  \
    XX(nanosleep)   \
    XX(socket)  \
    XX(connect) \
    XX(accept)  \
    XX(read)    \
    XX(readv)   \
    XX(recv)    \
    XX(recvfrom)\
    XX(recvmsg) \
    XX(write)   \
    XX(writev)  \
    XX(send)    \
    XX(sendto)  \
    XX(sendmsg) \
    XX(close)   \
    XX(fcntl)   \
    XX(ioctl)   \
    XX(getsockopt)  \
    XX(setsockopt)

void hook_init()
{
    static bool is_inited=false;
    if(is_inited){
        return ;
    }
#define XX(name) name ## _f=(name ## _fun)dlsym(RTLD_NEXT,#name);   //dlsym():实现动态库链接的拦截和重新映射,通常用于获取函数符号地址，这样可用于对共享库中函数的包装
                                                                    //RTLD_NEXT:返回的就是第一个遇到(匹配上) sleep 这个符号的函数的函数地址
    HOOK_FUN(XX);                                                   
                                                                    
#undef XX
}


static uint64_t s_connect_timeout = -1;

struct _HookIniter  //要在main之前取出系统库的源码
{
    _HookIniter()
    {
        hook_init();    //在全局变量数据初始化的时候执行函数(在main函数之前执行)
        s_connect_timeout=g_tcp_connect_timeout->getValue();

        g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value)
        {
            HCSERVER_LOG_INFO(g_logger) << "tcp connect timeout changed from  "<< old_value << " to " << new_value;
            s_connect_timeout = new_value;
        });
    }
};
    
static _HookIniter s_hook_initer;   //静态全局变量在mian函数之前调用_HookIniter构造方法，初始化数据



//返回当前线程是否hook
bool is_hook_enable()
{
    return t_hook_enable;
}      
//设置当前线程的hook状态        
void set_hook_enable(bool flag)
{
    t_hook_enable=flag;
}

}




struct timer_info   //建立条件
{
    int cancelled=0;    //存储共享指针的引用数
};

//在IO异步操作中，是要使用原来的那些IO函数还是要使用自己重新编写的函数呢？
//重新编写要hook的函数模板(socket、io、fd相关)
//fd :句柄  fun: 和socket、io、fd相关的函数   hook_fun_name :要hook的函数名(字符串)
//event :在IOmanager的事件  timeout_so :在fd_manager的超时类型
//args :fun函数的参数
//(有超时时间，就加个定时器(等待时间)，超出等待时间后执行对应事件函数，并退出)
//(函数与IOManager的事件绑定，使得调用的时候触发)
template<typename OriginFun,typename... Args>
static ssize_t do_io(int fd,OriginFun fun,const char* hook_fun_name,uint32_t event,int timeout_so,Args&&... args)
{   
    //不用hook
    if(!HCServer::t_hook_enable)   //当前线程不采用hook
    {   //forward(完美转发)按照参数原来的类型转发到另一个函数
        return fun(fd,forward<Args>(args)...); //什么都不做，把原函数返回回去
    }
    //HCSERVER_LOG_INFO(g_logger)<<"do_io<"<<hook_fun_name<<">";
        
    HCServer::FdCtx::ptr ctx=HCServer::FdMgr::Getinstance()->get(fd);  //获取/创建文件句柄类FdCtx
    if(!ctx)    //文件句柄不存在
    {   //forward(完美转发)按照参数原来的类型转发到另一个函数
        return fun(fd,forward<Args>(args)...); //什么都不做，把原函数返回回去
    }

    if(ctx->isClose())  //如果文件句柄已关闭
    {
        errno=EBADF;
        return -1;
    }

    if(!ctx->isSocket() || ctx->getUserNonblock())  //不是socket 或 是用户主动设置的非阻塞(hook)
    {   //forward(完美转发)按照参数原来的类型转发到另一个函数
        return fun(fd,forward<Args>(args)...); //什么都不做，把原函数返回回去
    }

    //hook要做的
    uint64_t to=ctx->getTimeout(timeout_so);    //获取超时时间,用于后面添加一个定时器，实现异步操作
    shared_ptr<timer_info> tinfo(new timer_info);  //建立条件

retry:  //失败继续读取标签

    //先执行一遍原来的函数，如read、write、accept等和IO相关的函数，成功则直接返回n
    ssize_t n=fun(fd,forward<Args>(args)...); 
    while (n==-1 && errno==EINTR)   //失败原因：处于中断
    {
        n=fun(fd,forward<Args>(args)...);
    }
    if(n==-1 && errno==EAGAIN)  //失败原因：处于非阻塞(目前没有可用数据输入)，要加定时器，要做异步操作
    {
        
        HCServer::IOManager* iom=HCServer::IOManager::Getthis();  //IO
        HCServer::Timer::ptr timer;
        weak_ptr<timer_info> winfo(tinfo); //创建一个winfo条件————借助weak_ptr类型指针可以获取shared_ptr指针的一些状态信息(比如引用计数)

        if(to!=(uint64_t)-1)    //没有超时,要加条件定时器
        {
            timer=iom->addConditiomTimer(to,[winfo,fd,iom,event]()
            {   //添加条件定时器(如果对应的共享指针存在，执行定时器回调函数)
                auto t=winfo.lock();    //返回winfo对应的存储空共享指针(timer_info)
                if(!t | t->cancelled)   //没有触发定时器    cancelled判断是否设错误
                {
                    return;
                }
                t->cancelled=ETIMEDOUT; //ETIMEDOUT超时
                iom->cancelEvent(fd,(HCServer::IOManager::Event)(event));  //超时，则执行函数
            },winfo);
        }

        int rt=iom->addEvent(fd,(HCServer::IOManager::Event)(event));  //添加IOmanager事件，等待可读或者可写后再做事情
        if(HCSERVER_UNLIKELY(rt))  //添加失败
        {
            HCSERVER_LOG_ERROR(g_logger)<<hook_fun_name<<" addEvent("<<fd<<", "<<event<<")";
            if(timer)
            {
                timer->cancel();    //删除定时器
            }
            return -1;
        }else//添加成功
        {
            //HCSERVER_LOG_INFO(g_logger)<<"do_io<"<<hook_fun_name<<">";
            HCServer::Fiber::YieldToHold();   //将当前协程切换到后台，并将主协程切换出来,并设置为HOLD状态
            //HCSERVER_LOG_INFO(g_logger)<<"do_io<"<<hook_fun_name<<">";
            if (timer){
                timer->cancel();    //删除定时器
            }
            if(tinfo->cancelled)    //ETIMEDOUT超时
            {
                errno=tinfo->cancelled;
                return -1;
            }
            goto retry; //失败
        }
    }
    return n;
}


extern "C"{
#define XX(name) name ## _fun name ## _f=nullptr;   //(声明)函数
    HOOK_FUN(XX);
#undef XX



unsigned int sleep(unsigned int seconds)//重新编写sleep函数
{
    if(!HCServer::t_hook_enable){
        return sleep_f(seconds);
    }
    HCServer::Fiber::ptr fiber=HCServer::Fiber::getthis();
    HCServer::IOManager* iom=HCServer::IOManager::Getthis();
    iom->addTimer(seconds*1000,bind((void(HCServer::Scheduler::*) //(void(HCServer::Scheduler::*)(HCServer::Fiber::ptr,int thread))定义bind模板方法
                (HCServer::Fiber::ptr,int thread))& HCServer::IOManager::schedule //thread是函数有默认值
                ,iom,fiber,-1));
    HCServer::Fiber::YieldToHold();
    return 0;
}

int usleep (useconds_t usec)//重新编写和sleep差不多的函数—————微秒级
{
    if(!HCServer::t_hook_enable){
        return usleep_f(usec);
    }
    HCServer::Fiber::ptr fiber=HCServer::Fiber::getthis();
    HCServer::IOManager* iom=HCServer::IOManager::Getthis();
    iom->addTimer(usec/1000,bind((void(HCServer::Scheduler::*) //(void(HCServer::Scheduler::*)(HCServer::Fiber::ptr,int thread))定义bind模板方法
                (HCServer::Fiber::ptr,int thread))& HCServer::IOManager::schedule //thread是函数有默认值
                ,iom,fiber,-1));
    HCServer::Fiber::YieldToHold();
    return 0;
}

int nanosleep(const struct timespec *req,struct timespec *rem)  //重新编写和sleep差不多的函数————毫秒级
{
    if(!HCServer::t_hook_enable)
    {
        return nanosleep_f(req,rem);    //返回原来函数
    }

    int timeout_ms=req->tv_sec*1000+req->tv_nsec/1000/1000; //tv_sec是秒    tv_nsec是纳秒
    HCServer::Fiber::ptr fiber=HCServer::Fiber::getthis();    //返回当前协程
    HCServer::IOManager* iom=HCServer::IOManager::Getthis();  //返回当前iomanager
    iom->addTimer(timeout_ms,bind((void(HCServer::Scheduler::*)   //(void(sylar::Scheduler::*)(sylar::Fiber::ptr,int thread))定义bind模板方法
                (HCServer::Fiber::ptr,int thread))& HCServer::IOManager::schedule //thread是函数有默认值
                ,iom,fiber,-1));
    HCServer::Fiber::YieldToHold();    //切换到主协程
    return 0;
}
//socket
int socket(int domain,int type,int protocol)    //重新编写socket，覆盖原本的系统函数
{
    if(!HCServer::t_hook_enable)   //当前线程不采用hook
    {
        return socket_f(domain,type,protocol);
    }
    int fd=socket_f(domain,type,protocol);
    if(fd==-1)
    {
        return fd;
    }
    HCServer::FdMgr::Getinstance()->get(fd,true);  //单例模式创建FdManager对象，获取文件句柄类FdCtx————自动创建，并插入容器
    return fd;
}

int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms)   //重新编写和connect差不多的函数(增加了超时变量timeout_ms)
{

    if(!HCServer::t_hook_enable)   //当前线程不采用hook
    {
        return connect_f(fd, addr, addrlen);
    }
    HCServer::FdCtx::ptr ctx = HCServer::FdMgr::Getinstance()->get(fd);   //获取文件句柄类FdCtx————自动创建，并插入容器
    if(!ctx || ctx->isClose())  //不存在，已关闭
    {
        errno = EBADF;
        return -1;
    }

    if(!ctx->isSocket())    //不是socket
    {
        return connect_f(fd, addr, addrlen);
    }

    if(ctx->getUserNonblock())  //用户主动设置的非阻塞 
    {
        return connect_f(fd, addr, addrlen);
    }

    //hook操作,加定时器，绑定I/O事件
    int n = connect_f(fd, addr, addrlen);
    if(n == 0)
    {
        return 0;
    }else if(n != -1 || errno != EINPROGRESS)    //以非阻塞的方式来进行连接
    {
        return n;
    }

    HCServer::IOManager* iom = HCServer::IOManager::Getthis();
    HCServer::Timer::ptr timer;
    shared_ptr<timer_info> tinfo(new timer_info);
    weak_ptr<timer_info> winfo(tinfo);//创建一个条件

    if(timeout_ms != (uint64_t)-1)  //没有超时，加条件定时器
    {
        timer = iom->addConditiomTimer(timeout_ms, [winfo, fd, iom]()
        {   //添加条件定时器(如果对应的共享指针存在，执行定时器回调函数)
            auto t = winfo.lock();  //返回winfo对应的存储空共享指针(timer_info)
            if(!t || t->cancelled)  //没有触发定时器    cancelled：超时标志
            {
                return;
            }
            t->cancelled = ETIMEDOUT;   //ETIMEDOUT超时
            iom->cancelEvent(fd, HCServer::IOManager::WRITE);  //定时器超时时，则执行fd对应的事件的写事件
        }, winfo);
    }

    int rt = iom->addEvent(fd, HCServer::IOManager::WRITE);//添加IOmanager事件
    if(rt == 0)//添加成功
    {
        HCServer::Fiber::YieldToHold();    //将当前协程切换到后台，并将主协程切换出来
        if(timer)
        {
            timer->cancel();    //删除定时器
        }
        if(tinfo->cancelled)
        {   
            errno = tinfo->cancelled;
            return -1;
        }
    }else//添加失败
    {
        if(timer)
        {
            timer->cancel();    //删除定时器
        }
        HCSERVER_LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
    }
    
    int error = 0;
    socklen_t len = sizeof(int);
    if(-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len))    //取出socket状态
    {
        return -1;
    }
    if(!error)
    {
        return 0;
    }else{
        errno = error;
        return -1;
    }
}

//connect
int connect(int sockfd,const struct sockaddr* addr,socklen_t addrlen)   //重新编写connect，覆盖原本的系统函数
{
    return connect_with_timeout(sockfd, addr, addrlen, HCServer::s_connect_timeout);   //和connect差不多的函数(增加了超时变量)
}

//accept
int accept(int s,struct sockaddr* addr,socklen_t* addrlen)  //服务器接收连接
{
    int fd=do_io(s,accept_f,"accept",HCServer::IOManager::READ,SO_RCVTIMEO,addr,addrlen);  //有超时时间
    if(fd>=0)
    {
        HCServer::FdMgr::Getinstance()->get(fd,true);
    }
    return fd;
}

//read
ssize_t read(int fd, void *buf, size_t count)
{
    return do_io(fd,read_f,"read",HCServer::IOManager::READ,SO_RCVTIMEO,buf,count); 
}
//readv
ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
    return do_io(fd,readv_f,"readv",HCServer::IOManager::READ,SO_RCVTIMEO,iov,iovcnt);
}
//recv
ssize_t recv(int sockfd, void *buf, size_t len, int flags)
{
    return do_io(sockfd,recv_f,"recv",HCServer::IOManager::READ,SO_RCVTIMEO,buf,len,flags);
}
//recvfrom
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,struct sockaddr *src_addr, socklen_t *addrlen)
{
    return do_io(sockfd,recvfrom_f,"recvfrom",HCServer::IOManager::READ,SO_RCVTIMEO,buf,len,flags,src_addr,addrlen);
}
//recvmsg
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
{
     return do_io(sockfd,recvmsg_f,"recvmsg",HCServer::IOManager::READ,SO_RCVTIMEO,msg,flags);
}


//write
ssize_t write(int fd, const void *buf, size_t count)    //重新编写write，覆盖原本的系统函数
{
    return do_io(fd,write_f,"write",HCServer::IOManager::WRITE,SO_SNDTIMEO,buf,count); //SO_SNDTIMEO设置socket发送数据超时时间
}
//writev
ssize_t writev(int fd, const struct iovec *iov, int iovcnt) //重新编写writev，覆盖原本的系统函数
{
    return do_io(fd,writev_f,"writev",HCServer::IOManager::WRITE,SO_SNDTIMEO,iov,iovcnt);
}
//send
ssize_t send(int sockfd, const void *buf, size_t len, int flags)    //重新编写send，覆盖原本的系统函数
{
    return do_io(sockfd,send_f,"send",HCServer::IOManager::WRITE,SO_SNDTIMEO,buf,len,flags);
}
//sendto
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,const struct sockaddr *dest_addr, socklen_t addrlen)  //重新编写sendto，覆盖原本的系统函数
{
    return do_io(sockfd,sendto_f,"sendto",HCServer::IOManager::WRITE,SO_SNDTIMEO,buf,len,flags,dest_addr,addrlen);
}
//sendmsg
ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags)    //重新编写sendmsg，覆盖原本的系统函数
{
    return do_io(sockfd,sendmsg_f,"sendmsg",HCServer::IOManager::WRITE,SO_SNDTIMEO,msg,flags);
}


//close
int close(int fd)   //重新编写close，覆盖原本的系统函数
{
    if(!HCServer::t_hook_enable)   //当前线程不采用hook
    {
        return close_f(fd);
    }
    HCServer::FdCtx::ptr ctx=HCServer::FdMgr::Getinstance()->get(fd); //获取文件句柄类FdCtx————自动创建，并插入容器
    if(ctx)
    {
        auto iom=HCServer::IOManager::Getthis();
        if(iom)
        {
            iom->cancelAll(fd); //取消所有事件
        }
        HCServer::FdMgr::Getinstance()->del(fd);   //删除文件句柄类
    }
}



//socket操作(fd) 相关的
//fcntl
int fcntl(int fd, int cmd, ... /* arg */ )  //重新编写fcntl，覆盖原本的系统函数
{
    va_list va;
    va_start(va,cmd);  //宏初始化变量
    switch(cmd)
    {
        //int
        case F_SETFL:
            {
                int arg=va_arg(va,int); //返回可变的参数
                va_end(va); //清空
                HCServer::FdCtx::ptr ctx=HCServer::FdMgr::Getinstance()->get(fd); //获取文件句柄类FdCtx————自动创建，并插入容器
                if(!ctx || ctx->isClose() || !ctx->isSocket())  //不存在，已关闭，不是socket
                {
                    return fcntl_f(fd,cmd,arg);
                }
                ctx->setSysNonblock(arg & O_NONBLOCK);  //O_NONBLOCK非阻塞
                if(ctx->getSysNonblock()){
                        arg |= O_NONBLOCK;
                }
                return fcntl_f(fd,cmd,arg);
            }
            break;
        case F_DUPFD:
            {
                va_end(va);
                int arg = fcntl_f(fd, cmd);
                HCServer::FdCtx::ptr ctx = HCServer::FdMgr::Getinstance()->get(fd);   //获取文件句柄类FdCtx————自动创建，并插入容器
                if(!ctx || ctx->isClose() || !ctx->isSocket())  //不存在，已关闭，不是socket
                {
                    return arg;
                }
                if(ctx->getUserNonblock()) {
                    return arg | O_NONBLOCK;
                } else {
                    return arg & ~O_NONBLOCK;
                }
            }
            break;
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
        case F_SETPIPE_SZ: 
            {
                int arg=va_arg(va,int); //返回可变的参数
                va_end(va); //清空
                return fcntl_f(fd,cmd,arg);
            }           
            break;

        //void
        case F_GETFD:
        case F_GETFL:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
        case F_GETPIPE_SZ:
            {
                va_end(va); //清空
                return fcntl_f(fd,cmd);   
            }
            break;

        //struct flock *
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
            {
                struct flock* arg=va_arg(va,struct flock*); //返回可变的参数
                va_end(va); //清空
                return fcntl_f(fd,cmd,arg);
            }
            break;

        //struct f_owner_ex *
        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                struct f_owner_exlock* arg=va_arg(va,struct f_owner_exlock*);   //返回可变的参数
                va_end(va); //清空
                return fcntl_f(fd,cmd,arg);
            }
            break;
        default:
            va_end(va); //清空
            return fcntl_f(fd,cmd); 
    }
}

//ioctl
int ioctl(int d, unsigned long int request, ...)  //重新编写ioctl，覆盖原本的系统函数(ioctl控制接口函数)
{
    va_list va;
    va_start(va, request);  //宏初始化变量
    void* arg = va_arg(va, void*);  //返回可变的参数
    va_end(va); //清空

    if(FIONBIO == request)
    {
        bool user_nonblock = !!*(int*)arg;  //是否阻塞
        HCServer::FdCtx::ptr ctx = HCServer::FdMgr::Getinstance()->get(d);    //获取文件句柄类FdCtx————自动创建，并插入容器
        if(!ctx || ctx->isClose() || !ctx->isSocket())  //不存在，已关闭，不是socket
        {
            return ioctl_f(d, request, arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_f(d, request, arg);
}

//getsockopt
int getsockopt(int sockfd, int level, int optname,void *optval, socklen_t *optlen)  //重新编写getsockopt，覆盖原本的系统函数(getsockopt获取一个套接字的选项)
{
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

//setsockopt
int setsockopt(int sockfd, int level, int optname,const void *optval, socklen_t optlen) //为网络套接字设置选项值，比如：允许重用地址、网络超时 
{
    if(!HCServer::t_hook_enable)   //当前线程不采用hook
    {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    if(level == SOL_SOCKET)
    {
        if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)    //SO_RCVTIMEO接受超时   SO_SNDTIMEO发送超时
        {
            HCServer::FdCtx::ptr ctx = HCServer::FdMgr::Getinstance()->get(sockfd);   //获取文件句柄类FdCtx————自动创建，并插入容器
            if(ctx)
            {
                const timeval* v = (const timeval*)optval;
                ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000); //设置超时时间——毫秒
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}


}

