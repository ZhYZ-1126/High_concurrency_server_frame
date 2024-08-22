#include"../HCServer/HCServer.h"
#include<unistd.h>
#include<iostream>

HCServer::Logger::ptr g_logger=HCSERVER_LOG_ROOT();

int a=0;

//HCServer::RWMutex rw_mutex;
HCServer::Mutex s_mutex;

void func1(){
    HCSERVER_LOG_INFO(g_logger)<<"name="<<HCServer::Thread::Getname()
                                <<" this.name="<<HCServer::Thread::GetThis()->getname()
                                <<" id="<<HCServer::GetthreadId()
                                <<" this.id="<<HCServer::Thread::GetThis()->Getid();
    for(int i=0;i<500000;i++){
        //HCServer::RWMutex::Writelock lock(rw_mutex);
        HCServer::Mutex::Lock lock(s_mutex);
        a++;
    }
}
void func2(){
    while(true){
        HCSERVER_LOG_INFO(g_logger)<<"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}
void func3(){
    while(true){
        HCSERVER_LOG_INFO(g_logger)<<"===================================";
    }
}
int main(int argc,char ** argv)
{
    HCSERVER_LOG_INFO(g_logger)<<"thread test begin";
    YAML::Node root=YAML::LoadFile("/home/zhangyanzhou/HCServer/bin/conf/log2.yml"); 
    HCServer::Config::LoadFileYaml(root);
    vector<HCServer::Thread::ptr> thrs;

    // //创建线程
    // HCServer::Thread::ptr th1(new HCServer::Thread(&func1,"name_"+to_string(1)));
    // thrs.push_back(th1);
    for(int i=0;i<2;++i){
        

        HCServer::Thread::ptr th2(new HCServer::Thread(&func2,"name_"+to_string(i*2)));
        HCServer::Thread::ptr th3(new HCServer::Thread(&func3,"name_"+to_string(i*2+1)));
        thrs.push_back(th2);
        thrs.push_back(th3);
    }
    for(size_t i=0;i<thrs.size();++i){
        thrs[i]->join();
    }
    HCSERVER_LOG_INFO(g_logger)<<"thread test end"<<endl;
    //HCSERVER_LOG_INFO(g_logger)<<"a="<<a<<endl;
    
    return 0;
}