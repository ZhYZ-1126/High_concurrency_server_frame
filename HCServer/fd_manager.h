#ifndef _HCSERVER_FD_MANAGER_H__
#define _HCSERVER_FD_MANAGER_H__

#include<memory>
#include<vector>
#include"thread.h"
#include"singleton.h"
#include"iomanager.h"


namespace HCServer{

//fd上下文类
class FdCtx:public enable_shared_from_this<FdCtx>
{
public:
    typedef shared_ptr<FdCtx> ptr;
    FdCtx(int fd);
    ~FdCtx();

    bool init();//初始化

    bool isInit() const { return m_isInit;} //获取是否初始化完成
    bool isSocket() const { return m_isSocket;} //获取是否socket
    bool isClose() const { return m_isClosed;}  //获取是否已关闭

    void setUserNonblock(bool v) { m_userNonblock = v;}//设置用户主动设置非阻塞(用户态)

    bool getUserNonblock() const { return m_userNonblock;}  //获取是否用户主动设置的非阻塞

    void setSysNonblock(bool v) { m_sysNonblock = v;} //设置系统非阻塞(系统态)
    bool getSysNonblock() const { return m_sysNonblock;}    //获取系统非阻塞

    void setTimeout(int type, uint64_t v);//设置超时时间  
                                    //type 类型SO_RCVTIMEO(读超时), SO_SNDTIMEO(写超时) v 时间毫秒  

    uint64_t getTimeout(int type);//获取超时时间


private:
    bool m_isInit : 1;//是否初始化
    bool m_isSocket : 1;//是否是socket
    bool m_sysNonblock : 1;//是否为系统非阻塞
    bool m_userNonblock : 1;//是否为用户非阻塞
    bool m_isClosed : 1;//是否已经被关闭
    int m_fd;//文件句柄

    uint64_t m_recvTimeout;//读超时时间毫秒cv
    uint64_t m_sendTimeout;//写超时时间毫秒
};
//fd管理类
class FdManager
{
public:
    typedef RWMutex RWMutexType;

    FdManager();

    //fd 文件句柄  auto_create 是否自动创建 (返回对应文件句柄类FdCtx::ptr)
    FdCtx::ptr get(int fd,bool auto_create=false);//获取/创建文件句柄类FdCtx


    void del(int fd);//删除文件句柄

private:
    RWMutexType m_mutex;    //读写锁
    vector<FdCtx::ptr> m_datas;    //文件句柄集合
};
typedef Singleton<FdManager> FdMgr;



}




#endif