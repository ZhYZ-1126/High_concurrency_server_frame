#include"../HCServer/HCServer.h"
#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include <arpa/inet.h>
#include<fcntl.h>
#include<sys/epoll.h>
#include<iostream>
#include<unistd.h>

using namespace std;

HCServer::Logger::ptr g_logger=HCSERVER_LOG_ROOT();

void test_sleep()
{
    HCServer::IOManager iom(1,false);
    iom.schedule([] () {
        sleep(2);
        HCSERVER_LOG_INFO(g_logger)<<"sleep2";
    });
    iom.schedule([] () {
        sleep(3);
        HCSERVER_LOG_INFO(g_logger)<<"sleep3";
    });

    HCSERVER_LOG_INFO(g_logger)<<"test_sleep";
}

void test_socket()
{
    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    //fcntl(sockfd,F_SETFL,O_NONBLOCK);

    sockaddr_in serveraddr;//服务器的地址
    memset(&serveraddr,0,sizeof(serveraddr));
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port=htons(80);
    // serveraddr.sin_addr.s_addr=htonl(INADDR_ANY);
    inet_pton(AF_INET,"183.2.172.42",&serveraddr.sin_addr.s_addr);

    HCSERVER_LOG_INFO(g_logger)<<"begin connect" ;
    int rt=connect(sockfd,(const sockaddr*)&serveraddr,sizeof(serveraddr));
    HCSERVER_LOG_INFO(g_logger)<<"connect rt="<<rt<< " errno="<<errno;

    if(rt)
    {
        return;
    }

    const char buf[]="GET / HTTP/1.0\r\n\r\n ";
    rt=send(sockfd,buf,sizeof(buf),0);
    HCSERVER_LOG_INFO(g_logger)<<"send rt="<<rt<< " errno="<<errno;
    if(rt<=0)
    {
        return ;
    }

    string buff;
    buff.resize(4096);
    rt=recv(sockfd,&buff[0],buff.size(),0);
    HCSERVER_LOG_INFO(g_logger)<<"recv rt="<<rt<< " errno="<<errno;
    if(rt<=0)
    {
        return ;
    }
    buff.resize(rt);
    HCSERVER_LOG_INFO(g_logger)<<buff;
}


int main()
{
    //test_sleep();
    HCServer::IOManager iom(1,false);
    iom.schedule(&test_socket);
    sleep(2);
    cout<<"================================"<<endl;
    return 0;
}