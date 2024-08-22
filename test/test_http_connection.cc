#include <iostream>
#include "../HCServer/http/http_connection.h"
#include "../HCServer/log.h"
#include "../HCServer/iomanager.h"
#include "../HCServer/http/http_parser.h"
#include <fstream>
using namespace std;

static HCServer::Logger::ptr g_logger = HCSERVER_LOG_ROOT();

void test_pool()
{
    HCServer::http::HttpConnectionPool::ptr pool(new HCServer::http::HttpConnectionPool(
                "www.sylar.top", "", 80, false, 10, 1000 * 30, 5));

    HCServer::IOManager::Getthis()->addTimer(1000, [pool](){
            auto r = pool->doGet("/", 300); //发送HTTP的GET请求(字符串uri)
            HCSERVER_LOG_INFO(g_logger) << r->toString();
    }, true);
}

void run()
{
    HCServer::Address::ptr addr = HCServer::Address::LookupAnyIPAddress("www.sylar.top:80");  //通过host地址返回对应条件的任意IPAddress
    if(!addr)
    {
        HCSERVER_LOG_INFO(g_logger) << "get addr error";
        return;
    }

    HCServer::Socket::ptr sock = HCServer::Socket::CreateTCP(addr);   //利用Address创建TCP Socket
    bool rt = sock->connect(addr);  //客户端连接服务器地址
    if(!rt)
    {
        HCSERVER_LOG_INFO(g_logger) << "connect " << *addr << " failed";
        return;
    }

    HCServer::http::HttpConnection::ptr conn(new HCServer::http::HttpConnection(sock));   //HTTP客户端类(处理客户端和服务器的交流)
    HCServer::http::HttpRequest::ptr req(new HCServer::http::HttpRequest);    //HTTP请求结构体
    req->setPath("/blog/"); //设置HTTP请求的路径
    req->setHeader("host", "www.sylar.top");    //设置HTTP请求的头部参数
    HCSERVER_LOG_INFO(g_logger) << "req:" << endl
        << *req;

    conn->sendRequest(req); //发送HTTP请求
    auto rsp = conn->recvResponse();    //接收HTTP响应，解析响应协议

    if(!rsp)
    {
        HCSERVER_LOG_INFO(g_logger) << "recv response error";
        return;
    }
    HCSERVER_LOG_INFO(g_logger) << "rsp:" << endl
        << *rsp;

    ofstream ofs("rsp.dat");
    ofs << *rsp;

    HCSERVER_LOG_INFO(g_logger) << "=========================";

    auto r = HCServer::http::HttpConnection::DoGet("http://www.sylar.top/blog/", 300); //发送HTTP的GET请求(字符串uri)
    HCSERVER_LOG_INFO(g_logger) << "result=" << r->result
        << " error=" << r->error
        << " rsp=" << (r->response ? r->response->toString() : "");

    HCSERVER_LOG_INFO(g_logger) << "=========================";
    test_pool();
}

// void test_https()
// {
//     auto r = HCServer::http::HttpConnection::DoGet("http://www.baidu.com/", 300,{
//                         {"Accept-Encoding", "gzip, deflate, br"},
//                         {"Connection", "keep-alive"},
//                         {"User-Agent", "curl/7.29.0"}
//             });
//     HCSERVER_LOG_INFO(g_logger) << "result=" << r->result
//         << " error=" << r->error
//         << " rsp=" << (r->response ? r->response->toString() : "");

//     //HCServer::http::HttpConnectionPool::ptr pool(new HCServer::http::HttpConnectionPool(
//     //            "www.baidu.com", "", 80, false, 10, 1000 * 30, 5));
//     auto pool = HCServer::http::HttpConnectionPool::Create(
//                     "https://www.baidu.com", "", 10, 1000 * 30, 5);
//     HCServer::IOManager::GetThis()->addTimer(1000, [pool](){
//             auto r = pool->doGet("/", 3000, {
//                         {"Accept-Encoding", "gzip, deflate, br"},
//                         {"User-Agent", "curl/7.29.0"}
//                     });
//             HCSERVER_LOG_INFO(g_logger) << r->toString();
//     }, true);
// }

// void test_data()
// {
//     HCServer::Address::ptr addr = HCServer::Address::LookupAny("www.baidu.com:80");
//     auto sock = HCServer::Socket::CreateTCP(addr);

//     sock->connect(addr);
//     const char buff[] = "GET / HTTP/1.1\r\n"
//                 "connection: close\r\n"
//                 "Accept-Encoding: gzip, deflate, br\r\n"
//                 "Host: www.baidu.com\r\n\r\n";
//     sock->send(buff, sizeof(buff));

//     string line;
//     line.resize(1024);

//     ofstream ofs("http.dat", ios::binary);
//     int total = 0;
//     int len = 0;
//     while((len = sock->recv(&line[0], line.size())) > 0)
//     {
//         total += len;
//         ofs.write(line.c_str(), len);
//     }
//     cout << "total: " << total << " tellp=" << ofs.tellp() << endl;
//     ofs.flush();
// }

// void test_parser()
// {
//     ifstream ifs("http.dat", ios::binary);
//     string content;
//     string line;
//     line.resize(1024);

//     int total = 0;
//     while(!ifs.eof())
//     {
//         ifs.read(&line[0], line.size());
//         content.append(&line[0], ifs.gcount());
//         total += ifs.gcount();
//     }

//     cout << "length: " << content.size() << " total: " << total << endl;
//     HCServer::http::HttpResponseParser parser;
//     size_t nparse = parser.execute(&content[0], content.size(), false);
//     cout << "finish: " << parser.isFinished() << endl;
//     content.resize(content.size() - nparse);
//     cout << "rsp: " << *parser.getData() << endl;

//     auto& client_parser = parser.getParser();
//     string body;
//     int cl = 0;
//     do {
//         size_t nparse = parser.execute(&content[0], content.size(), true);
//         cout << "content_len: " << client_parser.content_len
//                   << " left: " << content.size()
//                   << endl;
//         cl += client_parser.content_len;
//         content.resize(content.size() - nparse);
//         body.append(content.c_str(), client_parser.content_len);
//         content = content.substr(client_parser.content_len + 2);
//     } while(!client_parser.chunks_done);

//     cout << "total: " << body.size() << " content:" << cl << endl;

//     HCServer::ZlibStream::ptr stream = HCServer::ZlibStream::CreateGzip(false);
//     stream->write(body.c_str(), body.size());
//     stream->flush();

//     body = stream->getResult();

//     ofstream ofs("http.txt");
//     ofs << body;
// }

int main(int argc, char** argv) 
{
    HCServer::IOManager iom(2);
    iom.schedule(run);
    //iom.schedule(test_https);
    return 0;
}