//  HTTP客户端封装
//(处理数据的传输)
#ifndef __HCSERVER_HTTP_CONNECTION_H__
#define __HCSERVER_HTTP_CONNECTION_H__

//#include "HCServer-master/streams/socket_stream.h"
#include"../streams/socket_stream.h"
#include "http.h"
#include "../uri.h"
#include "../thread.h"

#include <list>

namespace HCServer
{
namespace http
{

//HTTP响应结果结构体
struct HttpResult
{
    typedef shared_ptr<HttpResult> ptr;    //智能指针类型定义

    //错误码定义
    enum class Error
    {
        OK = 0,             //正常
        INVALID_URL = 1,    //非法URL
        INVALID_HOST = 2,   //无法解析HOST
        CONNECT_FAIL = 3,   //连接失败
        SEND_CLOSE_BY_PEER = 4, //连接被对端关闭
        SEND_SOCKET_ERROR = 5,  //发送请求产生Socket错误
        TIMEOUT = 6,            //超时
        CREATE_SOCKET_ERROR = 7,    //创建Socket失败
        POOL_GET_CONNECTION = 8,    //从连接池中取连接失败
        POOL_INVALID_CONNECTION = 9,//无效的连接
    };

    //构造函数(初始化) _result 错误码   _response HTTP响应结构体    _error 错误描述
    HttpResult(int _result,HttpResponse::ptr _response,const string& _error)
                :result(_result)
                ,response(_response)
                ,error(_error) 
    {}

    string toString() const;   //以字符串的形式输出错误码

    int result; //错误码
    HttpResponse::ptr response; //HTTP响应结构体
    string error;  //错误描述

    
};


class HttpConnectionPool;

//  HTTP客户端封装
//(处理数据的传输)
class HttpConnection : public SocketStream
{
friend class HttpConnectionPool;
public:

    typedef shared_ptr<HttpConnection> ptr;    //HTTP客户端类智能指针

    //发送HTTP的GET请求(字符串uri)
    //url 请求的url(字符串)     timeout_ms 超时时间(毫秒)   headers HTTP请求头部参数    body 请求消息体
    static HttpResult::ptr DoGet(const string& url, uint64_t timeout_ms
                    , const map<string, string>& headers = {}, const string& body = "");

    //发送HTTP的GET请求(结构体uri) 
    //uri URI结构体     timeout_ms 超时时间(毫秒)   headers HTTP请求头部参数    body 请求消息体
    static HttpResult::ptr DoGet(Uri::ptr uri, uint64_t timeout_ms
                    , const map<string, string>& headers = {}, const string& body = "");

    //发送HTTP的POST请求(字符串uri)
    //url 请求的url(字符串)     timeout_ms 超时时间(毫秒)   headers HTTP请求头部参数    body 请求消息体
    static HttpResult::ptr DoPost(const string& url, uint64_t timeout_ms
                            , const map<string, string>& headers = {}, const string& body = "");

    //发送HTTP的POST请求(结构体uri)
    //uri URI结构体     timeout_ms 超时时间(毫秒)   headers HTTP请求头部参数    body 请求消息体
    static HttpResult::ptr DoPost(Uri::ptr uri, uint64_t timeout_ms
                        , const map<string, string>& headers = {}, const string& body = "");

    //发送HTTP请求(字符串uri)
    //method 请求类型(请求方法)   url 请求的url(字符串)   timeout_ms 超时时间(毫秒)   headers HTTP请求头部参数  body 请求消息体
    static HttpResult::ptr DoRequest(HttpMethod method, const string& url, uint64_t timeout_ms 
                        ,const map<string, string>& headers = {}, const string& body = "");

    //发送HTTP请求(结构体uri)
    //method 请求类型(请求方法)  uri URI结构体   timeout_ms 超时时间(毫秒)   headers HTTP请求头部参数    body 请求消息体
    static HttpResult::ptr DoRequest(HttpMethod method, Uri::ptr uri, uint64_t timeout_ms
                        , const map<string, string>& headers = {}, const string& body = "");

    //发送HTTP请求(HttpRequest请求结构体、结构体uri)
    //req 请求结构体    uri URI结构体   timeout_ms 超时时间(毫秒)
    static HttpResult::ptr DoRequest(HttpRequest::ptr req, Uri::ptr uri, uint64_t timeout_ms);

    //构造函数      sock Socket类   owner 是否掌握控制权
    HttpConnection(Socket::ptr sock, bool owner = true);
    ~HttpConnection();

    HttpResponse::ptr recvResponse(); //接收HTTP响应，解析响应协议

    int sendRequest(HttpRequest::ptr req);//发送HTTP请求

private:
    uint64_t m_createTime = 0;  //创建时间
    uint64_t m_request = 0; //链接请求次数
};


//HTTP客户端连接池
//一个host————1：1————连接池————1：n————多个HTTP客户端类与host服务器链接
// (长连接，连接池中链接都有时间限制，请求次数限制)
//(从连接池中获取链接(域名)，向链接发送uri字符串path?query#fragment请求)
class HttpConnectionPool
{
public:    
    typedef shared_ptr<HttpConnectionPool> ptr;    //智能指针
    typedef Mutex MutexType;    //锁

    //创建HttpConnectionPool(HTTP客户端连接池)
    //url 请求的url(字符串)  vhost 自定义host   max_size 连接池最多能存放多少条链接
    //max_alive_time 链接存在时间上限   max_request 链接请求次数上限
    static HttpConnectionPool::ptr Create(const string& uri,const string& vhost,uint32_t max_size
                            ,uint32_t max_alive_time,uint32_t max_request);

    //构造函数 
    HttpConnectionPool(const string& host,const string& vhost,uint32_t port,bool is_https
                ,uint32_t max_size,uint32_t max_alive_time,uint32_t max_request);

    HttpConnection::ptr getConnection();//从连接池取出链接，没有，则自动根据host创建链接

    //发送HTTP的GET请求(字符串uri)
    // url 请求的url(字符串)   path?query#fragment  timeout_ms 超时时间(毫秒)
    // headers HTTP请求头部参数     body 请求消息体
    HttpResult::ptr doGet(const string& url, uint64_t timeout_ms, const map<string, string>& headers = {}
                                , const string& body = "");

    //发送HTTP的GET请求(结构体uri)
    // url URI结构体     timeout_ms 超时时间(毫秒)
    // headers HTTP请求头部参数     body 请求消息体
    HttpResult::ptr doGet(Uri::ptr uri, uint64_t timeout_ms, const map<string, string>& headers = {}
                        , const string& body = "");

    //发送HTTP的POST请求(字符串uri)
    // url 请求的url(字符串)   path?query#fragment  timeout_ms 超时时间(毫秒)
    // headers HTTP请求头部参数     body 请求消息体
    HttpResult::ptr doPost(const string& url, uint64_t timeout_ms
                , const map<string, string>& headers = {}, const string& body = "");

    //发送HTTP的POST请求(结构体uri)
    // url URI结构体     timeout_ms 超时时间(毫秒)
    // headers HTTP请求头部参数     body 请求消息体
    HttpResult::ptr doPost(Uri::ptr uri, uint64_t timeout_ms, const map<string, string>& headers = {}
                                , const string& body = "");

    //发送HTTP请求(字符串uri)  
    //method 请求类型(请求方法)     uri 请求的url(字符串)
    //timeout_ms 超时时间(毫秒)     headers HTTP请求头部参数    body 请求消息体
    HttpResult::ptr doRequest(HttpMethod method, const string& url, uint64_t timeout_ms
                , const map<string, string>& headers = {}, const string& body = "");

    //发送HTTP请求(结构体uri)
    //method 请求类型(请求方法)     uri URI结构体
    //timeout_ms 超时时间(毫秒)     headers HTTP请求头部参数    body 请求消息体
    HttpResult::ptr doRequest(HttpMethod method, Uri::ptr uri, uint64_t timeout_ms
                , const map<string, string>& headers = {}, const string& body = "");

    //发送HTTP请求
    //req 请求结构体    timeout_ms 超时时间(毫秒)
    HttpResult::ptr doRequest(HttpRequest::ptr req, uint64_t timeout_ms);
        
private:
    static void ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool);  //释放连接池指针()

private:
    string m_host; //默认域名
    string m_vhost;    //自定义域名
    uint32_t m_port;    //端口号
    uint32_t m_maxSize; //连接池最多能存放多少条链接
    uint32_t m_maxAliveTime;    //链接存在时间上限
    uint32_t m_maxRequest;  //链接请求次数上限
    bool m_isHttps; //是否是https协议，不是则是http协议

    MutexType m_mutex;  //互斥量
    list<HttpConnection*> m_conns; //连接池
    atomic<int32_t> m_total = {0}; //当前连接池连接条数
};




}
}

#endif