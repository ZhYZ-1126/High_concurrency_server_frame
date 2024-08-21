#include"../HCServer/tcp_server.h"
#include"../HCServer/iomanager.h"

static HCServer::Logger::ptr g_logger=HCSERVER_LOG_ROOT();

void test()
{
    auto addr=HCServer::Address::LookupAny("0.0.0.0:8033");
    //auto addr1= HCServer::UnixAddress::ptr(new HCServer::UnixAddress("/tmp/unix_addr"));
    vector<HCServer::Address::ptr> addrs;
    vector<HCServer::Address::ptr> fails;
    addrs.push_back(addr);
    //addrs.push_back(addr1);
    HCServer::TcpServer::ptr tcp_server(new HCServer::TcpServer);
    while(!tcp_server->bind(addrs,fails)){
        sleep(2);
    }
    tcp_server->start();
}    


int main()
{
    HCServer::IOManager iom(2);
    iom.schedule(&test);
    return 0;
}