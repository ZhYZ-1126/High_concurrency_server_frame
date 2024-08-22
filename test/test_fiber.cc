#include"../HCServer/HCServer.h"
#include"../HCServer/thread.h"

#include<memory>
using namespace std;


HCServer::Logger::ptr g_logger=HCSERVER_LOG_ROOT();

//子协程
void run_in_fiber()
{
    HCSERVER_LOG_INFO(g_logger)<<"run_in_fiber begin";
    HCServer::Fiber::YieldToHold();//交出协程执行权，切回主协程，并且将状态置为HOLD
    //sleep(2);
    HCSERVER_LOG_INFO(g_logger)<<"run_in_fiber end";
    HCServer::Fiber::YieldToHold();  
}
//主协程
void test_fiber()
{
    {
        HCServer::Fiber::getthis();
        HCSERVER_LOG_INFO(g_logger)<<"main_fiber begin";
        HCServer::Fiber::ptr fiber(new HCServer::Fiber(run_in_fiber,0,true));//创建子协程
        fiber->call();//执行子协程，拿到执行权
        HCSERVER_LOG_INFO(g_logger)<<"main after swapIn";
        fiber->call();//执行子协程，拿到执行权
        HCSERVER_LOG_INFO(g_logger)<<"main end";
        fiber->call();//执行子协程，拿到执行权
    }
    HCSERVER_LOG_INFO(g_logger)<<"main end1";
}
int main(){
    //HCServer::Thread::SetName("main");//设置线程的名称
    vector<HCServer::Thread::ptr> thread;//创建线程容器
    for(int i=0;i<3;++i){
        thread.push_back(HCServer::Thread::ptr(new HCServer::Thread(&test_fiber,"name_"+to_string(i))));//创建子线程
    }
    for(auto a:thread){
        a->join();//阻塞主线程，当子线程a结束后才能够执行主线程
    }
    return 0;
}


