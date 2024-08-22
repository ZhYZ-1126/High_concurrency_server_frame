#include"../HCServer/iomanager.h"
#include"../HCServer/HCServer.h"
//#include"../HCServer/log.h"
#include<sys/types.h>
//#include <string.h>
#include<sys/socket.h>
#include <arpa/inet.h>
#include<fcntl.h>
#include<sys/epoll.h>
#include<iostream>
#include<unistd.h>

using namespace std;

HCServer::Logger::ptr g_logger=HCSERVER_LOG_ROOT();

int sockfd=0;

void test_fiber()
{
    HCSERVER_LOG_INFO(g_logger)<<"test_fiber sockfd="<<sockfd;
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    fcntl(sockfd,F_SETFL,O_NONBLOCK);

    sockaddr_in serveraddr;//服务器的地址
    memset(&serveraddr,0,sizeof(serveraddr));
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port=htons(80);
    //serveraddr.sin_addr.s_addr=htonl(INADDR_ANY);
    inet_pton(AF_INET,"100.72.244.231",&serveraddr.sin_addr.s_addr);
    
    if(connect(sockfd,(const sockaddr *)&serveraddr,sizeof(serveraddr))==-1){
       HCSERVER_LOG_INFO(g_logger)<<"connect failed";
    }else {//if(errno==EINPROGRESS){
        cout<<"sjduiadhuiyqwhduiawhd"<<endl;
        HCSERVER_LOG_INFO(g_logger)<<" add event errno="<<errno<<" "<<strerror(errno);
        HCServer::IOManager::Getthis()->addEvent(sockfd,HCServer::IOManager::READ, [](){
            HCSERVER_LOG_INFO(g_logger)<<"read callback";
        });
        HCServer::IOManager::Getthis()->addEvent(sockfd,HCServer::IOManager::WRITE, [](){
            HCSERVER_LOG_INFO(g_logger)<<"write callback";
            HCServer::IOManager::Getthis()->cancelEvent(sockfd,HCServer::IOManager::READ);
            close(sockfd);
        });
    }
    // else{
    //     HCSERVER_LOG_ERROR(g_logger)<<"else "<<errno<<" "<<strerror(errno);

    // }   
}
void test_iomanager()
{   
    cout<<"EPOLLIN="<<EPOLLIN<<" EPOLLOUT="<<EPOLLOUT<<endl;
    HCServer::IOManager iom(2,false);
    iom.schedule(&test_fiber);//往协程任务队列中添加任务
}

HCServer::Timer::ptr t_timer;
void test_timer()
{
    HCServer::IOManager iom(2);
    t_timer=iom.addTimer(1000,[](){
        static int i=0;
        HCSERVER_LOG_INFO(g_logger)<<"hello timer i="<<i;
        if(++i==3){
            //t_timer->cancel();//取消定时器t_timer
            t_timer->reset(2000,HCServer::GetcurrentMS());//重置定时器的时间间隔和时间
        }
    },true);
}

int main()
{
    //test_iomanager();
    test_timer();
    return 0;
}