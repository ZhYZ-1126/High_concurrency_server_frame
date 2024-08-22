#include"../HCServer/tcp_server.h"
#include"../HCServer/socket.h"
#include"../HCServer/bytearray.h"
#include"../HCServer/HCServer.h"

static HCServer::Logger::ptr g_logger=HCSERVER_LOG_ROOT();

class EchoServer:public HCServer::TcpServer
{
public:
    EchoServer(int type);
    virtual void handleClient(HCServer::Socket::ptr client) override;  //处理新连接的Socket类(处理新连接的客户端数据传输)
private:
    int m_type=0;
};
EchoServer::EchoServer(int type)
:m_type(type)
{}

void EchoServer::handleClient(HCServer::Socket::ptr client)
{
    HCSERVER_LOG_INFO(g_logger)<<"handleClient "<<*client;
    HCServer::ByteArray::ptr ba(new HCServer::ByteArray);
    while(true){
        ba->clear();
        vector<iovec> iovs;
        ba->getWriteBuffers(iovs,1024);

        int rt=client->recv(&iovs[0],iovs.size());
        if(rt==0){
            HCSERVER_LOG_INFO(g_logger)<<"client closs : " <<*client;
            break;
        }else if(rt<0){
            HCSERVER_LOG_INFO(g_logger)<<"client error rt = " <<rt
                    << " errno=" <<errno<<" errstr="<<strerror(errno);
            break;
        }
        ba->setPosition(ba->getPosition()+rt);

        ba->setPosition(0);//方便tostring()，从起始位置开始读
        if(m_type==1){//text
            cout<<ba->toString();
        }else{
            cout<<ba->toHexString();
        }
        std::cout.flush();
    }
}

int type=1;
void run()
{
    EchoServer::ptr es(new EchoServer(type));
    auto addr=HCServer::Address::LookupAny("0.0.0.0:8020");
    while(!es->bind(addr)){
        sleep(2);
    }
    es->start();
}


int main(int argc,char **argv)
{
    if(argc<2)
    {
        HCSERVER_LOG_INFO(g_logger)<<"used as["<<argv[0]<<" -t] or ["<<argv[0]<<" -b]";
        return 0;
    }
    if(!strcmp(argv[1],"-b")){
        type=2;
    }
    HCServer::IOManager iom(2);
    iom.schedule(run);
    return 0;
}