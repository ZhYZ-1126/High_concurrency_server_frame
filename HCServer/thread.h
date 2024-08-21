#ifndef _HCSERVER_THREAD_H__
#define _HCSERVER_THREAD_H__

#include<thread>
#include<functional>
#include<memory>
#include<pthread.h>
#include<semaphore.h>
#include<stdint.h>
#include<atomic>
#include"noncopyable.h"

using namespace std; 

namespace HCServer{

//信号量类
class Semaphore: Noncopyable{
public:    
    Semaphore(uint32_t count=0);
    ~Semaphore();

    void wait();
    void notify();
private:
    // //禁止默认拷贝
    // Semaphore(const Semaphore&)=delete;
    // Semaphore(const Semaphore&&)=delete;
    // Semaphore& operator=(const Semaphore&)=delete;
private:
    sem_t m_semaphore;//定义一个信号量
};

//互斥量模板类
template<class T>
class ScopedLockImpl{
public:
    ScopedLockImpl(T& mutex)
    :m_mutex(mutex)
    {
        m_mutex.lock();
        m_locked=true;
    }
    ~ScopedLockImpl(){
        unlock();
    }
    void lock(){
        if(!m_locked){
            m_mutex.lock();
            m_locked=true;
        }
    }
    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked=false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;//false:互斥量已上锁，true：互斥量未上锁

};

//读锁类
template<class T>
class ReadScopedLockImpl{
public:
    ReadScopedLockImpl(T& mutex)
    :m_mutex(mutex)
    {
        m_mutex.rdlock();//加读锁
        m_locked=true;
    }
    ~ReadScopedLockImpl(){
        unlock();
    }
    void lock(){
        if(!m_locked){
            m_mutex.rdlock();//加读锁
            m_locked=true;
        }
    }
    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked=false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;//false:互斥量已上锁，true：互斥量未上锁

};

//写锁类
template<class T>
class WriteScopedLockImpl{
public:
    WriteScopedLockImpl(T& mutex)
    :m_mutex(mutex)
    {
        m_mutex.wrlock();//加写锁
        m_locked=true;
    }
    ~WriteScopedLockImpl(){
        unlock();
    }
    void lock(){
        if(!m_locked){
            m_mutex.wrlock();//加写锁
            m_locked=true;
        }
    }
    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked=false;
        }
    }
private:
    T& m_mutex;
    bool m_locked;//false:互斥量未上锁，true：互斥量已上锁

};

//互斥量类
class Mutex: Noncopyable{
public:
    typedef ScopedLockImpl<Mutex> Lock;

    Mutex(){
        pthread_mutex_init(&m_mutex,nullptr);
    }
    ~Mutex(){
        pthread_mutex_destroy(&m_mutex);
    }
    void lock(){
        pthread_mutex_lock(&m_mutex);
    }
    void unlock(){
        pthread_mutex_unlock(&m_mutex);
    }
private:
    pthread_mutex_t m_mutex;
};

class NullMutex: Noncopyable{
public:
    typedef ScopedLockImpl<Mutex> Lock;
    NullMutex();
    ~NullMutex();
    void lock();
    void unlock();
};

//读写互斥量类
class RWMutex: Noncopyable{
public:
    typedef ReadScopedLockImpl<RWMutex> Readlock;
    typedef WriteScopedLockImpl<RWMutex> Writelock;
    RWMutex(){
        pthread_rwlock_init(&m_lock,nullptr);// 初始化
    }
    ~RWMutex(){
        pthread_rwlock_destroy(&m_lock);
    }
    void rdlock(){
        pthread_rwlock_rdlock(&m_lock);// 加读锁
    }
    void wrlock(){
        pthread_rwlock_wrlock(&m_lock);// 加写锁
    }
    void unlock(){
        pthread_rwlock_unlock(&m_lock);// 释放锁
    }
private:
    pthread_rwlock_t m_lock;//读写锁
};

// class RWNullMutex{
// public:
//     typedef ReadScopedLockImpl<RWNullMutex> lock;
//     typedef WriteScopedLockImpl<RWNullMutex> lock; 
//     RWNullMutex();
//     ~RWNullMutex();
//     void lock();
//     void unlock();
// };

//spinlock类
class Spinlock: Noncopyable{
public:
    typedef ScopedLockImpl<Spinlock> Lock;

    Spinlock(){
        pthread_spin_init(&m_mutex,0);
    }
    ~Spinlock(){
        pthread_spin_destroy(&m_mutex);
    }
    void lock(){
        pthread_spin_lock(&m_mutex);
    }
    void unlock(){
        pthread_spin_unlock(&m_mutex);
    }
private:
    pthread_spinlock_t m_mutex;
};

//CASLock
class CASLock: Noncopyable{
public:
    typedef ScopedLockImpl<CASLock> Lock;

    CASLock(){
        m_mutex.clear();
    }
    ~CASLock(){}
    void lock(){
        while(atomic_flag_test_and_set_explicit(&m_mutex,memory_order_acquire));
    }
    void unlock(){
        atomic_flag_clear_explicit(&m_mutex,memory_order_release);
    }
private:
    volatile atomic_flag m_mutex;
};

//线程类
class Thread{
public: 
    typedef shared_ptr<Thread> ptr;
    Thread(function<void()> cb,const string & name);
    ~Thread();

    pid_t Getid() const{return m_id;}
    const string& getname() const {return m_name;}

    void join();
    static Thread *GetThis();//获取当前线程，提供给那些需要获取当前线程的系统使用
    static const string& Getname();//获取当然线程的名称，该方法供日志使用
    static void SetName(const string& name);
private:
    //禁止默认拷贝
    Thread(const Thread&)=delete;
    Thread(const Thread&&)=delete;
    Thread& operator=(const Thread&)=delete;

    static void * run(void *arg);//线程执行函数
private:
    pid_t m_id=-1;//线程id
    pthread_t m_thread=0;//线程标识符
    function<void()> m_cb;//回调
    string m_name;//线程名称
    Semaphore m_semaphore;//信号量
};


}


#endif