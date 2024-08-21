#include"thread.h"
#include"log.h"
#include"util.h"
#include"hook.h"
using namespace std;

namespace HCServer{

static thread_local Thread* t_thread=nullptr;//线程局部变量，指向当前线程
static thread_local string t_thread_name="UNKNOW";//当前线程名称

static HCServer::Logger::ptr g_logger=HCSERVER_LOG_NAME("system");


Semaphore::Semaphore(uint32_t count)//count为信号量的初始值
{
    if(sem_init(&m_semaphore,0,count)){
        throw logic_error("sem_init error");
    }
}
Semaphore::~Semaphore()
{
    sem_destroy(&m_semaphore);
}
void Semaphore::wait()
{
    //sem_wait()作用是使信号量的值减去一个“1”，但它永远会先等待该信号量为一个非零值才开始做减法。
    //如果信号量的值为0则阻塞等待，如果信号量的值大于0则将信号量的值减1，立即返回。
    if(sem_wait(&m_semaphore)){
        throw logic_error("sem_wait error");
    }
}
void Semaphore::notify()
{
    //释放信号量，让信号量的值加1
    if(sem_post(&m_semaphore)){
        throw logic_error("sem_post error");
    }
}

//获取当前线程，提供给那些需要获取当前线程的系统使用
Thread *Thread::GetThis()
{
    return t_thread;
}
//获取当然线程的名称，该方法供日志使用
const string& Thread::Getname()
{
    return t_thread_name;
}
//设置线程名称
void Thread::SetName(const string& name)
{
    if(name.empty())
    {
        return;
    }
    if(t_thread){
        t_thread->m_name=name;
    }
    t_thread_name=name;
}
Thread::Thread(function<void()> cb,const string & name)
:m_cb(cb),m_name(name)
{
    if(name.empty()){
        m_name="UNKNOW";
    }
    int rt=pthread_create(&m_thread,nullptr,&Thread::run,this);//创建子线程，成功返回0，失败返回1，run是子线程开始运行的函数
    if(rt){
        HCSERVER_LOG_ERROR(g_logger)<<"pthread_create thread fail,rt="<<rt<<" name="<<name;
        throw logic_error("pthread_create error");
    }
    m_semaphore.wait();//信号量减1
}
Thread::~Thread(){
    if(m_thread){
        pthread_detach(m_thread);//将已经运行中的子线程设定为分离状态,实现该线程与当前线程分离
    }
}

void Thread::join()
{
    if(m_thread){
        //pthread_join:挂起当前线程，用于阻塞式地等待m_thread线程结束，如果m_thread线程已结束则立即返回，0=成功
        int rt=pthread_join(m_thread,nullptr);//等待m_thread线程运行结束之后再运行其他线程
        if(rt){
            HCSERVER_LOG_ERROR(g_logger)<<"pthread_join thread fail,rt="<<rt<<" name="<<m_name;
            throw logic_error("pthread_join error");
        }
        m_thread=0; 
    }
}

void * Thread::run(void *arg)
{
    Thread* thread=(Thread*)arg;
    t_thread=thread;
    t_thread_name=thread->m_name;
    thread->m_id=HCServer::GetthreadId();
    pthread_setname_np(pthread_self(),thread->m_name.substr(0,15).c_str());

    function<void()> cb;
    cb.swap(thread->m_cb);//互相交换，得到当前线程执行的回调函数

    thread->m_semaphore.notify();//信号量加1
    cb();//在run函数里面执行线程的回调函数，当run函数执行结束时会自动释放
    return 0;
}

}