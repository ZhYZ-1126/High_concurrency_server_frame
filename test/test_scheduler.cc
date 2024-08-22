#include"../HCServer/HCServer.h"
#include"../HCServer/scheduler.h"

HCServer::Logger::ptr g_logger=HCSERVER_LOG_ROOT();

void test_fiber()
{
    static int s_count=5;
    HCSERVER_LOG_INFO(g_logger)<<"test in fiber s_count="<<s_count;
    
    //sleep(1);
    if(--s_count>=0)
    {
        //HCServer::Scheduler::getthis()->schedule(&test_fiber);//添加协程任务，任务会在随机的线程中执行
        HCServer::Scheduler::getthis()->schedule(&test_fiber,HCServer::GetthreadId());//往调度器中添加协程任务，任务会在固定的线程中执行
    }
}

int main()
{
    HCSERVER_LOG_INFO(g_logger)<<"main";
    HCServer::Scheduler sc(3,false,"test");
    HCSERVER_LOG_INFO(g_logger)<<"before start";
    sc.start();
    sleep(2);
    HCSERVER_LOG_INFO(g_logger)<<"schedule";
    sc.schedule(&test_fiber);//往调度器中添加协程任务
    HCSERVER_LOG_INFO(g_logger)<<"before stop";
    sc.stop();
    HCSERVER_LOG_INFO(g_logger)<<"over";
    return 0;
}