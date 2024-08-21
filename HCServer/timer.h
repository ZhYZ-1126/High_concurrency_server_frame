#ifndef _HCSERVER_TIMER_H__
#define _HCSERVER_TIMER_H__

#include<memory>
#include"thread.h"
#include<vector>
#include<functional>
#include<set>
using namespace std;


namespace HCServer{
class TimerManager;//定时器管理类
//定时器类
class Timer :public enable_shared_from_this<Timer>
{
friend class TimerManager;
public:
    typedef shared_ptr<Timer> ptr;

    bool cancel();//取消
    bool refresh();//刷新
    bool reset(uint64_t ms,bool from_now);//重置时间和执行间隔

private:
    //执行周期，回调，是否循环定时器（循环一直进行），定时器管理类对象
    Timer(uint64_t ms,function<void ()> cb,bool recurring,TimerManager * manager);
    Timer(uint64_t next);

private:
    bool m_recurring=false; //是否循环定时器
    uint64_t m_ms=0;        //执行周期（循环定时器）
    uint64_t m_next=0;      //精确的执行时间
    function<void ()> m_cb; //定时器执行的回调函数
    TimerManager* m_timemanager=nullptr;

private:
    //实现定时器之间的比较，从而实现排序
    struct Comparator{
        bool operator()(const Timer::ptr & lhs,const Timer::ptr& rhs ) const;
    };
};

class TimerManager{
friend class Timer;
public:
    typedef RWMutex RWMutexType;

    TimerManager();
    virtual ~TimerManager();

    //添加定时器
    Timer::ptr addTimer(uint64_t ms,function<void ()> cb,bool recurring=false);
    //添加条件定时器,使用weak_ptr智能指针作为条件，如果该智能指针已经消失的话则无需执行该定时器
    Timer::ptr addConditiomTimer(uint64_t ms,function<void ()> cb,weak_ptr<void> weak_cond, bool recurring=false);

    uint64_t getNextTimer();//获得下一个定时器的执行时间
    void listExpiredCb(vector<function<void()>> &cbs );//获取需要执行的定时器的回调函数列表
    
    //判断是否还有定时器
    bool hasTimer();
protected:
    //当有新的定时器插入到存放定时器的容器的开始位置时,执行该函数
    virtual void onTimerInsertedAtFront()=0;
    //判断当前插入的timer定时器的位置是不是在存放定时器容器的开始位置
    void IsaddTimer(Timer::ptr timer,RWMutexType::Writelock &lock);

private:
    bool detectClockRollover(uint64_t now_ms);//检测服务器时间是否被调后了
private:
    RWMutexType m_mutex;
    set<Timer::ptr,Timer::Comparator> m_timers; //存放定时器的有序(按照执行时间从小到大排序)容器
    bool m_tickled=false; //是否触发onTimerInsetedAtFront
    uint64_t m_previousTime=0;   //原来的时间
};


}


#endif