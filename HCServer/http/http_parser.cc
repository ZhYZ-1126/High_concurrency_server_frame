#include "http_parser.h"
#include"../log.h"
#include"../config.h"
using namespace std;


namespace HCServer{

namespace http{

static HCServer::Logger::ptr g_logger=HCSERVER_LOG_ROOT();

//限制请求头部一个字段(一次发送的数据大小)为4K————超出4K则数据有问题
static HCServer::ConfigVar<uint64_t>::ptr g_http_request_buffer_size=
        HCServer::Config::Lookup("http.request.buffer_size",(uint64_t)(4 * 1024), "http request buffer size");
//限制请求消息体一个字段(一次发送的数据大小)为64M————超出64M则数据有问题
static HCServer::ConfigVar<uint64_t>::ptr g_http_request_max_body_size =
    HCServer::Config::Lookup("http.request.max_body_size",(uint64_t)(64 * 1024 * 1024), "http request max body size");
//限制响应头部一个字段(一次发送的数据大小)为4K————超出4K则数据有问题
static HCServer::ConfigVar<uint64_t>::ptr g_http_response_buffer_size =
    HCServer::Config::Lookup("http.response.buffer_size",(uint64_t)(4 * 1024), "http response buffer size");
//限制响应消息体一个字段(一次发送的数据大小)为64M————超出64M则数据有问题
static HCServer::ConfigVar<uint64_t>::ptr g_http_response_max_body_size =
            HCServer::Config::Lookup("http.response.max_body_size",(uint64_t)(64 * 1024 * 1024), "http response max body size");

static uint64_t s_http_request_buffer_size = 0;     //限制请求头部一个字段(一次发送的数据大小)为4K
static uint64_t s_http_request_max_body_size = 0;   //限制请求消息体一个字段(一次发送的数据大小)为64M
static uint64_t s_http_response_buffer_size = 0;    //限制响应头部一个字段(一次发送的数据大小)为4K
static uint64_t s_http_response_max_body_size = 0;  //限制响应消息体一个字段(一次发送的数据大小)为64M

uint64_t HttpRequestParser::GetHttpRequestBufferSize(){
    return s_http_request_buffer_size;
}

uint64_t HttpRequestParser::GetHttpRequestMaxBodySize(){
    return s_http_request_max_body_size;
}

uint64_t HttpResponseParser::GetHttpResponseBufferSize()
{
    return s_http_response_buffer_size;
}

uint64_t HttpResponseParser::GetHttpResponseMaxBodySize()
{
    return s_http_response_max_body_size;
}

struct _RequestSizeIniter {
    _RequestSizeIniter() {
        //初始化请求头部，请求消息体，响应头部，响应消息体的限制大小
        s_http_request_buffer_size = g_http_request_buffer_size->getValue();
        s_http_request_max_body_size = g_http_request_max_body_size->getValue();
        s_http_response_buffer_size = g_http_response_buffer_size->getValue();
        s_http_response_max_body_size = g_http_response_max_body_size->getValue();

        //添加监听，满足条件触发对限制大小的修改
        g_http_request_buffer_size->addListener(
                [](const uint64_t& ov, const uint64_t& nv){
                s_http_request_buffer_size = nv;
        });

        g_http_request_max_body_size->addListener(
                [](const uint64_t& ov, const uint64_t& nv){
                s_http_request_max_body_size = nv;
        });

        g_http_response_buffer_size->addListener(
                [](const uint64_t& ov, const uint64_t& nv){
                s_http_response_buffer_size = nv;
        });

        g_http_response_max_body_size->addListener(
                [](const uint64_t& ov, const uint64_t& nv){
                s_http_response_max_body_size = nv;
        });
    }
};
static _RequestSizeIniter _init;//在执行main函数之前先执行_RequestSizeIniter()构造函数，实现初始化



//对http request 报文解析的回调函数
void on_request_method(void *data,const char *at,size_t length)
{
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    HttpMethod m = CharsToHttpMethod(at);//将字符串指针转换成HTTP请求方法枚举

    if(m == HttpMethod::INVALID_METHOD) {//HTTP请求方法枚举类不存在m
        HCSERVER_LOG_WARN(g_logger) << "invalid http request method: "
            << string(at, length);
        parser->setError(1000);
        return;
    }
    parser->getData()->setMethod(m);//设置HTTP请求的方法(名称=序号  GET=1)
}
void on_request_uri(void *data,const char *at,size_t length)
{}
void on_request_fragment(void *data,const char *at,size_t length)
{
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);  //转换成HttpRequestParser  HTTP请求解析类
    parser->getData()->setFragment(string(at, length));
}
void on_request_path(void *data,const char *at,size_t length)
{
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);  //转换成HttpRequestParser  HTTP请求解析类
    parser->getData()->setPath(string(at, length));
}
void on_request_query(void *data,const char *at,size_t length)
{
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);  //转换成HttpRequestParser  HTTP请求解析类
    parser->getData()->setQuery(string(at, length));
}
void on_request_version(void *data,const char *at,size_t length)
{
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);  //转换成HttpRequestParser  HTTP请求解析类
    uint8_t v = 0;  //HTTP版本
    if(strncmp(at, "HTTP/1.1", length) == 0)    //strncmp判断字符串是否相等
    {
        v = 0x11;
    }else if(strncmp(at, "HTTP/1.0", length) == 0)
    {
        v = 0x10;
    }else{
        HCSERVER_LOG_WARN(g_logger) << "invalid http request version: "<< string(at, length);
        parser->setError(1001);
        return;
    }
    parser->getData()->setVersion(v);   //设置HTTP版本
}
void on_request_header_done(void *data,const char *at,size_t length)
{}
void on_request_http_field(void *data,const char *field,size_t flen,const char *value,size_t vlen)
{
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);  //转换成HttpRequestParser  HTTP请求解析类
    if(flen == 0)   //key为0
    {
        HCSERVER_LOG_WARN(g_logger) << "invalid http request field length == 0";
        return;
    }
    //设置HTTP请求的头部参数
    parser->getData()->setHeader(string(field, flen),string(value, vlen));
}

HttpRequestParser::HttpRequestParser()
:m_error(0)
{
    m_data.reset(new HCServer::http::HttpRequest);
    http_parser_init(&m_parser);
    //设置回调函数
    m_parser.request_method=on_request_method;
    m_parser.request_uri=on_request_uri;
    m_parser.fragment=on_request_fragment;
    m_parser.request_path=on_request_path;
    m_parser.query_string=on_request_query;
    m_parser.http_version=on_request_version;
    m_parser.header_done=on_request_header_done;
    m_parser.http_field=on_request_http_field;

    m_parser.data = this;

}

//解析Http请求协议  data 协议文本内存   len 协议文本内存长度
//return:返回实际解析的长度,并且将已解析的数据移除
size_t HttpRequestParser::execute(char* data, size_t len)
{
    size_t offset = http_parser_execute(&m_parser, data, len, 0);   //解析Http请求协议,返回实际解析的长度
    //从data的offset位置开始，移除(len - offset)长度的值到data，从data的第一个元素开始存放
    //memmove可以保证在内存发生局部重叠时的拷贝结果是正确的
    //len-offset：还未解析的数据长度
    memmove(data, data + offset, (len - offset));   //将已解析的数据移除,腾出内存空间    memmove()移动内存块
    return offset;
}

int HttpRequestParser::isFinished()  //是否解析完成
{
    return http_parser_finish(&m_parser);
}

int HttpRequestParser::hasError() //是否有错误 
{
    return m_error || http_parser_has_error(&m_parser);
}

uint64_t HttpRequestParser::getContentLength()    //获取消息体长度
{
    return m_data->getHeaderAs<uint64_t>("content-length", 0);
}




void on_response_reson(void *data,const char *at,size_t length)
{
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    parser->getData()->setReason(string(at, length));
}
void on_response_status(void *data,const char *at,size_t length)
{
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    HttpStatus status = (HttpStatus)(atoi(at));
    parser->getData()->setStatus(status);
}
void on_response_chunk(void *data,const char *at,size_t length)
{}
void on_response_version(void *data,const char *at,size_t length)
{
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    uint8_t v = 0;
    if(strncmp(at, "HTTP/1.1", length) == 0) {
        v = 0x11;
    } else if(strncmp(at, "HTTP/1.0", length) == 0) {
        v = 0x10;
    } else {
        HCSERVER_LOG_WARN(g_logger) << "invalid http response version: "
            << string(at, length);
        parser->setError(1001);
        return;
    }

    parser->getData()->setVersion(v);
}
void on_response_header_done(void *data,const char *at,size_t length)
{}
void on_response_last_chunk(void *data,const char *at,size_t length)
{}
void on_response_http_field(void *data,const char *field,size_t flen,const char *value,size_t vlen)
{
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    if(flen == 0) {
        HCSERVER_LOG_WARN(g_logger) << "invalid http response field length == 0";
        //parser->setError(1002);
        return;
    }
    parser->getData()->setHeader(string(field, flen),string(value, vlen));
}


HttpResponseParser::HttpResponseParser()
:m_error(0)
{
    m_data.reset(new HCServer::http::HttpResponse);
    httpclient_parser_init(&m_parser);
    //设置回调函数
    m_parser.reason_phrase=on_response_reson;
    m_parser.status_code=on_response_status;
    m_parser.chunk_size=on_response_chunk;
    m_parser.http_version=on_response_version;
    m_parser.header_done=on_response_header_done;
    m_parser.last_chunk=on_response_last_chunk;
    m_parser.http_field=on_response_http_field;

    m_parser.data = this;
}

//解析HTTP响应协议  data 协议数据内存   len 协议数据内存大小    chunck 是否在解析chunck(分块传输编码)
//return:返回实际解析的长度,并且移除已解析的数据
size_t HttpResponseParser::execute(char* data, size_t len, bool chunck)
{
    if(chunck) {
        httpclient_parser_init(&m_parser);
    }
    size_t offset = httpclient_parser_execute(&m_parser, data, len, 0);
    memmove(data, data + offset, (len - offset));
    return offset;
}

int HttpResponseParser::isFinished()   //是否解析完成
{
    return httpclient_parser_finish(&m_parser);
}

int HttpResponseParser::hasError()     //是否有错误
{
    return m_error || httpclient_parser_has_error(&m_parser);
}

uint64_t HttpResponseParser::getContentLength()    //获取消息体长度
{
    return m_data->getHeaderAs<uint64_t>("content-length", 0);
}



}

}