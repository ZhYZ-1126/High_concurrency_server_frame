#include "../HCServer/http/http.h"
#include "../HCServer/HCServer.h"
using namespace std;
void test_httprequest()
{
    HCServer::http::HttpRequest::ptr req(new HCServer::http::HttpRequest);
    req->setHeader("host","www.baidu.com");
    req->setBody("Hello zhangyanzhou");

    req->dump(cout)<<endl;
}

void test_httpresponse()
{
    HCServer::http::HttpResponse::ptr res(new HCServer::http::HttpResponse);
    res->setHeader("X-X","zhangyanzhou");
    res->setBody("hello zhangyanzhou");
    res->setStatus(HCServer::http::HttpStatus(400));
    res->setClose(false);

    res->dump(cout)<<endl;

}
int main()
{
    test_httprequest();
    test_httpresponse();
    return 0;
}