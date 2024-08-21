#include"../HCServer/socket.h"
#include"../HCServer/HCServer.h"
#include<iostream>

static HCServer::Logger::ptr g_logger=HCSERVER_LOG_ROOT();

void test_socket()
{
    HCServer::IPAddress::ptr addr= HCServer::Address::LookupAnyIPAddress("www.baidu.com",AF_INET);
    if(addr){
        HCSERVER_LOG_INFO(g_logger)<<"get address:"<<addr->toString();
    }else{
        HCSERVER_LOG_ERROR(g_logger)<<"get address fail";
    }
    //拿到地址后创建一个socket
    HCServer::Socket::ptr sock=HCServer::Socket::CreateTCP(addr);
    addr->setPort(80);
    HCSERVER_LOG_INFO(g_logger)<<"addr address after setport:"<<addr->toString();
    if(!sock->connect(addr)){
        HCSERVER_LOG_ERROR(g_logger)<<"connect "<<addr->toString()<<" fail";
        return ;
    }else{
        HCSERVER_LOG_INFO(g_logger)<<"connect "<<addr->toString()<<" success";
    }
    
    const char buff[]="GET / HTTP/1.0\r\n\r\n";
    int rt=sock->send(buff,sizeof(buff));
    if(rt<=0){
        HCSERVER_LOG_ERROR(g_logger)<<"send fail rt= "<< rt;
        return ;
    }
    string buffs;
    buffs.resize(4096);
    rt=sock->recv(&buffs[0],buffs.size());

    if(rt<0){
        HCSERVER_LOG_ERROR(g_logger)<<"recv fail rt= "<< rt;
        return ;
    }
    buffs.resize(rt);
    HCSERVER_LOG_INFO(g_logger)<<buffs;
}

int main()
{
    HCServer::IOManager iom;
    iom.schedule(&test_socket);
    return 0;
}