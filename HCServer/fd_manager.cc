#include"fd_manager.h"
#include"hook.h"
#include<sys/types.h>
#include<sys/stat.h>
#include<unistd.h>
#include<fcntl.h>

namespace HCServer{


FdCtx::FdCtx(int fd)
: m_isInit(false),m_isSocket(false),m_sysNonblock(false),m_userNonblock(false)
    ,m_isClosed(false),m_fd(fd),m_recvTimeout(-1),m_sendTimeout(-1)
{
    init();
}
FdCtx::~FdCtx()
{

}
bool FdCtx::init()
{
    if(m_isInit)
    {
        return true;
    }
    m_recvTimeout=-1;
    m_sendTimeout=-1;

    //stat:这个结构体是用来描述一个linux系统文件系统中的文件属性的结构
    struct stat fd_stat;    //看文件是不是句柄
    if (-1==fstat(m_fd,&fd_stat))   //失败 ,说明无法使用m_fd文件描述符打开文件 
    {
        m_isInit=false;
        m_isSocket=false;
    }else    //成功
    {
        m_isInit=true;
        m_isSocket=S_ISSOCK(fd_stat.st_mode);   //S_ISSOCK：判断是否是socket
    }

    if(m_isSocket)
    {
        int flags=fcntl_f(m_fd,F_GETFL,0);  //经常用这个fcntl函数对已打开的文件描述符改变为非阻塞，返回文件描述符状态(是否阻塞)
        if (!(flags & O_NONBLOCK))  //不是非阻塞
        {
            fcntl_f(m_fd,F_SETFL,flags | O_NONBLOCK);   //设置为非阻塞
        }
        m_sysNonblock=true;
    }else
    {
        m_sysNonblock=false;
    }

    m_userNonblock=false;
    m_isClosed=false;
    return m_isInit;        
}
//设置超时时间  type 类型SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时) v 时间毫秒  
void FdCtx::setTimeout(int type, uint64_t v)
{
    if(type==SO_RCVTIMEO)  //SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时)
    {
        m_recvTimeout=v;
    }else{
        m_sendTimeout=v;
    }
}
//获取超时时间
uint64_t FdCtx::getTimeout(int type)
{
    if(type==SO_RCVTIMEO){
        return m_recvTimeout;
    }else{
        return m_sendTimeout;
    }
}
FdManager::FdManager()
{
    m_datas.resize(64);
}
//获取/创建文件句柄类FdCtx
FdCtx::ptr FdManager::get(int fd,bool auto_create)
{
    if(fd==-1){
        return nullptr;
    }
    RWMutexType::Readlock lock(m_mutex);
    if((int)m_datas.size()<=fd)  //该fd不存在
    {
        if(auto_create==false)  //不自动创建
        {
            return nullptr;
        }
    }else    //fd存在或者不是自动创建
    {
        if(m_datas[fd] || !auto_create)
        {
            return m_datas[fd];
        }
    }
    lock.unlock();

    //自动创建，并插入容器
    RWMutexType::Writelock lock2(m_mutex);
    FdCtx::ptr ctx(new FdCtx(fd));
    if(fd>=(int)m_datas.size()) //容器大小不够，扩容
    {
        m_datas.resize(fd*1.5);
    }
    m_datas[fd]=ctx;
    return ctx;
}
//删除文件句柄
void FdManager::del(int fd)
{
    RWMutexType::Writelock lock(m_mutex);
    if((int)m_datas.size()<=fd)  //不存在
    {
        return;
    }
    m_datas[fd].reset();
}

}