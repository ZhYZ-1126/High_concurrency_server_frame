#include"../HCServer/http/http_parser.h"
#include"../HCServer/HCServer.h"

using namespace std;

static HCServer::Logger::ptr g_logger=HCSERVER_LOG_ROOT();
const char test_request_data[]="GET / HTTP/1.1\r\n" 
                         "Host:www.baidu.com\r\n"
                         "Content-Length:10\r\n\r\n"
                         "hello zhangyanzhou";

void test_request()
{
    HCServer::http::HttpRequestParser parser;
    string tmp=test_request_data;
    size_t rt=parser.execute(&tmp[0],tmp.size());
    HCSERVER_LOG_INFO(g_logger)<<"execute rt="<<rt<<" has_error="<<parser.hasError()
                                <<" is_finished="<<parser.isFinished()<<" total="<<tmp.size()
                                <<" context_length="<<parser.getContentLength();
    
    tmp.resize(tmp.size()-rt);//将已解析的数据移除
    HCSERVER_LOG_INFO(g_logger)<<parser.getData()->toString();
    HCSERVER_LOG_INFO(g_logger) << tmp;//输出移除已经解析的数据后的响应消息

}

const char test_response_data[]="HTTP/1.1 200 OK\r\n"
        "Date: Tue, 04 Jun 2019 15:43:56 GMT\r\n"
        "Server: Apache\r\n"
        "Last-Modified: Tue, 12 Jan 2010 13:48:00 GMT\r\n"
        "ETag: \"51-47cf7e6ee8400\"\r\n"
        "Accept-Ranges: bytes\r\n"
        "Content-Length: 81\r\n"
        "Cache-Control: max-age=86400\r\n"
        "Expires: Wed, 05 Jun 2019 15:43:56 GMT\r\n"
        "Connection: Close\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<html>\r\n"
        "<meta http-equiv=\"refresh\" content=\"0;url=http://www.baidu.com/\">\r\n"
        "</html>\r\n";

void test_response()
{
    HCServer::http::HttpResponseParser parser;
    string tmp=test_response_data;
    size_t rt = parser.execute(&tmp[0],tmp.size(), true);
    HCSERVER_LOG_INFO(g_logger)<<"execute rt="<<rt<<" has_error="<<parser.hasError()
                                <<" is_finished="<<parser.isFinished()<<" total="<<tmp.size()
                                <<" context_length="<<parser.getContentLength()<<" tmp[rt]="<<tmp[rt];

    tmp.resize(tmp.size() - rt); //将已解析的数据移除

    HCSERVER_LOG_INFO(g_logger) << parser.getData()->toString();
    HCSERVER_LOG_INFO(g_logger) << tmp;//输出移除已经解析的数据后的响应消息

}
int main()
{
    test_request();
    cout<<"-----------------------------"<<endl;
    test_response();
    return 0;
}