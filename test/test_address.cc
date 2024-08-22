#include"../HCServer/address.h"
#include"../HCServer/HCServer.h"
#include<map>
using namespace std;

HCServer::Logger::ptr g_logger=HCSERVER_LOG_ROOT();

void test()
{
    vector<HCServer::Address::ptr> addrs;
    HCSERVER_LOG_INFO(g_logger)<<"Lookup begin";
    //通过host地址（www.baidu.com）返回对应条件的所有Address,并插入容器中addrs
    bool v=HCServer::Address::Lookup(addrs,"www.baidu.com:80");
    HCSERVER_LOG_INFO(g_logger)<<"Lookup end";
    if(!v){
        HCSERVER_LOG_ERROR(g_logger)<<"Lookup fail";
        return ;
    }else{
        for(size_t i=0;i<addrs.size();++i){
            HCSERVER_LOG_INFO(g_logger)<<i<<" - "<<addrs[i]->toString();
        }
    }
}

void test_iface()
{
    multimap<string,pair<HCServer::Address::ptr,uint32_t> > results;
    //返回本机所有网卡的<网卡名, 地址, 子网掩码位数>到results
    bool v=HCServer::Address::GetInterfaceAddresses(results);
    if(!v){
        HCSERVER_LOG_INFO(g_logger)<<"GetInterfaceAddresses fail";
        return ;
    }
    for(auto& it:results){
        HCSERVER_LOG_INFO(g_logger)<<it.first<<" - "<<it.second.first->toString()<<" - "<<it.second.second;
    }
}

void test_createIpv4()
{
    auto addr=HCServer::IPAddress::Create("127.0.0.8",80);
    if(addr){
        HCSERVER_LOG_INFO(g_logger)<<addr->toString();
    }
}


int main()
{
    //test();
    //test_iface();
    //test_createIpv4();
    return 0;
}