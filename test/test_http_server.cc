#include"../HCServer/http/http_server.h"
#include"../HCServer/HCServer.h"

static HCServer::Logger::ptr g_logger=HCSERVER_LOG_ROOT();

void test_http_server()
{
    HCServer::http::HttpServer::ptr server(new HCServer::http::HttpServer);
    HCServer::Address::ptr addr=HCServer::Address::LookupAnyIPAddress("0.0.0.0:8020");
    while(!server->bind(addr))
    {
        sleep(2);
    }
    auto sd=server->getServletDispatch();
    sd->addServlet("/HCServer/xx",[](HCServer::http::HttpRequest::ptr req,
                HCServer::http::HttpResponse::ptr rsp,HCServer::http::HttpSession::ptr session){
                    rsp->setBody("hello zhangyanzhou:\r\n"+req->toString());
                    return 0;
                });
    
    sd->addGlobServlet("/HCServer/*",[](HCServer::http::HttpRequest::ptr req,
                HCServer::http::HttpResponse::ptr rsp,HCServer::http::HttpSession::ptr session){
                    rsp->setBody("Glob:\r\n"+req->toString());
                    return 0;
                });
    server->start();//启动服务器端，让线程去处理客户端的连接
}

int main(int argc,char ** argv)
{
    HCServer::IOManager iom(2);
    iom.schedule(test_http_server);
    return 0;
}