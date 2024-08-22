#include "timer.h"
#include "HCServer.h"

namespace HCServer{

bool Timer::Comparator::operator()(const Timer::ptr & lhs,const Timer::ptr& rhs )const
{
    if(!lhs &&!rhs){
        return false;
    }
    if(!lhs){
        return true;
    }
    if(!rhs){
        return false;
    }
    if(lhs->m_next<rhs->m_next){
        return true;
    }
    if(lhs->m_next>rhs->m_next){
        return false;
    }
    //如果时间一样则比较地址大小
    return lhs.get()<rhs.get();
}
Timer::Timer(uint64_t ms,function<void ()> cb,bool recurring,TimerManager * manager)
:m_ms(ms),m_cb(cb),m_recurring(recurring),m_timemanager(manager)
{
    m_next=HCServer::GetcurrentMS()+m_ms;//GetcurrentMS():获取当前时间
}

Timer::Timer(uint64_t next)
:m_next(next){}

//取消
bool Timer::cancel()
{
    TimerManager::RWMutexType::Writelock lock(m_timemanager->m_mutex);
    if(m_cb){
        m_cb=nullptr;
        auto it=m_timemanager->m_timers.find(shared_from_this());
        m_timemanager->m_timers.erase(it);
        return true;
    }
    return false;
}

//刷新
bool Timer::refresh()
{
    TimerManager::RWMutexType::Writelock lock(m_timemanager->m_mutex);
    if(!m_cb){
        return false;
    }
    auto it=m_timemanager->m_timers.find(shared_from_this());
    if(it==m_timemanager->m_timers.end()){
        return false;
    }
    m_timemanager->m_timers.erase(it);
    m_next=HCServer::GetcurrentMS()+m_ms;
    m_timemanager->m_timers.insert(shared_from_this());
    return true;
}

//重置时间和执行间隔
bool Timer::reset(uint64_t ms,bool from_now)
{
    if(ms==m_ms&&!from_now){
        return true;
    }

    TimerManager::RWMutexType::Writelock lock(m_timemanager->m_mutex);
    if(!m_cb){
        return false;
    }
    auto it=m_timemanager->m_timers.find(shared_from_this());
    if(it==m_timemanager->m_timers.end()){
        return false;
    }
    m_timemanager->m_timers.erase(it);
    uint64_t start=0;
    if(from_now){
        start=HCServer::GetcurrentMS();
    }else{
        start=m_next-m_ms;
    }
    m_ms=ms;
    m_next=start+m_ms;
    m_timemanager->IsaddTimer(shared_from_this(),lock);
    return true;
}
    
TimerManager::TimerManager()
{
    m_previousTime=HCServer::GetcurrentMS();
}

TimerManager::~TimerManager()
{

}

//添加定时器
Timer::ptr TimerManager::addTimer(uint64_t ms,function<void ()> cb,bool recurring)
{
    Timer::ptr timer(new Timer(ms,cb,recurring,this));
    RWMutexType::Writelock lock(m_mutex);
    IsaddTimer(timer,lock);
    return timer;
}

//对weak_cond指针是否被释放进行判断，如果没被释放则执行cb函数
static void OnTimer(weak_ptr<void> weak_cond,function<void()> cb)
{
    shared_ptr<void> tmp=weak_cond.lock();//lock():拿到当前weak_cond的指针
    if(tmp){//weak_cond智能指针还没被释放，执行cb函数
        cb();
    }
}

//添加条件定时器,使用weak_ptr智能指针作为条件，如果该智能指针已经消失的话则无需执行该定时器
Timer::ptr TimerManager::addConditiomTimer(uint64_t ms,function<void ()> cb,weak_ptr<void> weak_cond, bool recurring)
{
    return addTimer(ms,bind(&OnTimer,weak_cond,cb),recurring);
}
//获得下一个定时器的执行时间
uint64_t TimerManager::getNextTimer()
{
    RWMutexType::Readlock lock(m_mutex);
    m_tickled=false;
    if(m_timers.empty()){
        return ~0ull;
    }
    const Timer::ptr &next=*m_timers.begin();
    uint64_t now_ms=HCServer::GetcurrentMS();
    if(now_ms>=next->m_next){
        return 0;
    }else{
        return next->m_next-now_ms;
    }
}
//得到需要执行（定时器的执行时间超过当前的系统时间）的定时器的回调函数列表（cbs）
void TimerManager::listExpiredCb(vector<function<void()>> &cbs )
{
    uint64_t now_ms=HCServer::GetcurrentMS();
    vector<Timer::ptr> expireds;//存放已经超时的定时器
    {
        RWMutexType::Readlock lock(m_mutex);
        if(m_timers.empty()){
            return ;
        }
    }
    RWMutexType::Writelock lock(m_mutex);
    //detectClockRollover：检测服务器时间是否被调后了
    bool rollover=detectClockRollover(now_ms);
    if(!rollover && ((*m_timers.begin())->m_next >now_ms)){//定时器容器内的所有定时器的回调函数还未满足条件
        return ;
    }

    Timer::ptr now_timer(new Timer(now_ms));//创建一个执行时间为当时时间的一个定时器
    auto it=rollover ? m_timers.end(): m_timers.lower_bound(now_timer);//lower_bound,从头到尾查找第一个大于或等于now_timer的定时器，找到则返回该定时器的地址，
                                            //找不到则返回m_timers.end()。
    
    while (it!=m_timers.end() && (*it)->m_next==now_ms)
    {
        ++it;
    }
    expireds.insert(expireds.begin(),m_timers.begin(),it);    //插入执行时间超时的定时器
    m_timers.erase(m_timers.begin(),it);    //删除容器中超时的定时器
    cbs.reserve(expireds.size());    //更改vector的容量

    for (auto& timer : expireds) //把超时的定时器的回调函数放在cbs容器里
    {
        cbs.push_back(timer->m_cb);
        if (timer->m_recurring) //定时器循环使用
        {
            timer->m_next=now_ms+timer->m_ms;//该定时器下一次执行的时间
            m_timers.insert(timer);
        }else{
            timer->m_cb=nullptr;
        }
            
    }
}
//判断当前插入的timer定时器的位置是不是在存放定时器容器的开始位置
void TimerManager::IsaddTimer(Timer::ptr timer,RWMutexType::Writelock & lock)
{
     
    auto it=m_timers.insert(timer).first;
    bool at_front=(it==m_timers.begin()) && !m_tickled;
    if(at_front){
        m_tickled=true;
    }
    lock.unlock();

    if(at_front){//如果当前插入的位置是容器的开始位置
        onTimerInsertedAtFront();
    }
}
//判断是否还有定时器
bool TimerManager::hasTimer()
{
    RWMutexType::Readlock lock(m_mutex);
    return !m_timers.empty();
}
//检测服务器时间是否被调后了
bool TimerManager::detectClockRollover(uint64_t now_ms)
{
    bool rollover=false;
    if(now_ms<m_previousTime && now_ms<(m_previousTime-60*60*1000)){
        rollover=true;
    }
    m_previousTime=now_ms;
    return rollover;
}

}