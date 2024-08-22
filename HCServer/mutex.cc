#include"mutex.h"
#include"macro.h"
#include"scheduler.h"

namespace HCServer{

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


}